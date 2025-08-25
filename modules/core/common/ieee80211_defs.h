/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the IEEE802.11 protocol frame definitions.
//

#ifndef __IEEE80211_DEFS_H__
#define __IEEE80211_DEFS_H__

#include <stdint.h>
#include "neutrino_startpack.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

/*
 * 802.11 protocol definitions.
 */

#define IEEE80211_ADDR_LEN 6 /* size of 802.11 address */

/* IEEE 802.11 PLCP header */
struct ieee80211_plcp_hdr {
    uint16_t i_sfd;
    uint8_t i_signal;
    uint8_t i_service;
    uint16_t i_length;
    uint16_t i_crc;
} __ATTRIB_PACK;

#define IEEE80211_PLCP_SFD     0xF3A0
#define IEEE80211_PLCP_SERVICE 0x00

#define IEEE80211_NWID_LEN 32
#define IEEE80211_RSNX_LEN 3

typedef struct ssid_s {
    uint8_t ssid_len;
    uint8_t ssid[IEEE80211_NWID_LEN + 1];
} ssid_t;

/*
 * Authentication mode.
 */
enum ieee80211_authmode {
    IEEE80211_AUTH_NONE = 0,
    IEEE80211_AUTH_OPEN = 1,   /* open */
    IEEE80211_AUTH_SHARED = 2, /* shared-key */
    IEEE80211_AUTH_8021X = 3,  /* 802.1x */
    IEEE80211_AUTH_AUTO = 4,   /* auto-select/accept */
    /* NB: these are used only for ioctls */
    IEEE80211_AUTH_WPA = 5, /* WPA/RSN w/ 802.1x/PSK */
};

/*
 * 802.11 protocol crypto-related definitions.
 */

/* 802.11 HE IE info for 6G */
#define IEEE80211_HE_6G_INFO_PRESENT 0x20000

#define IEEE80211_KEYBUF_SIZE   16
#define IEEE80211_TX_MICKEY_LEN 8
#define IEEE80211_RX_MICKEY_LEN 8
/* space for both tx+rx keys */
#define IEEE80211_MICBUF_SIZE (IEEE80211_RX_MICKEY_LEN + IEEE80211_TX_MICKEY_LEN)

enum ieee80211_cipher_type {
    IEEE80211_CIPHER_WEP = 0,
    IEEE80211_CIPHER_TKIP = 1,
    IEEE80211_CIPHER_AES_OCB = 2,
    IEEE80211_CIPHER_AES_CCM = 3,
    IEEE80211_CIPHER_CKIP = 4,
    IEEE80211_CIPHER_NONE = 5, /* pseudo value */
#ifdef NT_FN_RMF
    IEEE80211_CIPHER_BIP = 6,
#endif  // NT_FN_RMF
};

#define IEEE80211_CIPHER_MAX (IEEE80211_CIPHER_NONE + 1)

typedef struct ieee80211_wep_key_s {
    uint8_t wk_keylen;
    uint8_t wk_key[IEEE80211_KEYBUF_SIZE];
} ieee80211_wep_key_t;

#define MAX_GROUP_KEY_INDEX 3

/*
 * 802.11 rate set.
 */
#define IEEE80211_RATE_SIZE       8  /* 802.11 standard */
#define IEEE80211_RATE_MAXSIZE    15 /* max rates we'll handle */
#define IEEE80211_MCS_RATE_MAXNUM 16 /* max rates we'll handle MCS 0-15*/

struct ieee80211_rateset {
    uint8_t rs_nrates;
    uint8_t rs_rates[IEEE80211_RATE_MAXSIZE];
};
struct ieee80211_mcsrateset {
    uint8_t rs_nrates;
    uint8_t rs_rates[IEEE80211_MCS_RATE_MAXNUM];
};

/*
 * 802.11g protection mode.
 */
enum ieee80211_protmode {
    IEEE80211_PROT_NONE = 0,    /* no protection */
    IEEE80211_PROT_CTSONLY = 1, /* CTS to self */
    IEEE80211_PROT_RTSCTS = 6,  /* RTS-CTS */
};

#define WMM_NUM_AC 4 /* 4 AC categories */

struct wmmParams {
    uint8_t wmmp_acm;        /* ACM parameter */
    uint8_t wmmp_aifsn;      /* AIFSN parameters */
    uint8_t wmmp_logcwmin;   /* cwmin in exponential form */
    uint8_t wmmp_logcwmax;   /* cwmax in exponential form */
    uint16_t wmmp_txopLimit; /* txopLimit */
};

struct chanAccParams {
    int8_t cap_info;                            /* ver. of the current param set */
    struct wmmParams cap_wmmParams[WMM_NUM_AC]; /*WMM params for each access class */
};

/*
 * generic definitions for IEEE 802.11 frames
 */
struct ieee80211_frame {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    uint8_t i_addr3[IEEE80211_ADDR_LEN];
    uint8_t i_seq[2];
    /* possibly followed by addr4[IEEE80211_ADDR_LEN]; */
    /* see below */
} __ATTRIB_PACK;

struct ieee80211_pspoll_frame {
    uint8_t i_fc[2];
    uint8_t i_aid[2];
    uint8_t i_bssid[IEEE80211_ADDR_LEN];
    uint8_t i_transAddr[IEEE80211_ADDR_LEN];
} __ATTRIB_PACK;

struct ieee80211_qosframe {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    uint8_t i_addr3[IEEE80211_ADDR_LEN];
    uint8_t i_seq[2];
    uint8_t i_qos[2];
} __ATTRIB_PACK;

struct ieee80211_qoscntl {
    uint8_t i_qos[2];
};

struct ieee80211_frame_addr4 {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    uint8_t i_addr3[IEEE80211_ADDR_LEN];
    uint8_t i_seq[2];
    uint8_t i_addr4[IEEE80211_ADDR_LEN];
} __ATTRIB_PACK;

struct ieee80211_qosframe_addr4 {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    uint8_t i_addr3[IEEE80211_ADDR_LEN];
    uint8_t i_seq[2];
    uint8_t i_addr4[IEEE80211_ADDR_LEN];
    uint8_t i_qos[2];
} __ATTRIB_PACK;

/* Management MIC information element (IEEE 802.11w) */
struct ieee80211_mmie {
    uint8_t element_id;
    uint8_t length;
    uint16_t key_id;
    // uint8_t sequence_number[6];
    uint64_t sequence_number : 48;
    uint8_t mic[8];
} __ATTRIB_PACK;

/* Management MIC information element (IEEE 802.11w) */
struct ieee80211_aad {
    uint8_t i_fc[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    uint8_t i_addr3[IEEE80211_ADDR_LEN];
} __ATTRIB_PACK;

#define IEEE80211_FC0_VERSION_MASK  0x03
#define IEEE80211_FC0_VERSION_SHIFT 0
#define IEEE80211_FC0_VERSION_0     0x00
#define IEEE80211_FC0_TYPE_MASK     0x0c
#define IEEE80211_FC0_TYPE_SHIFT    2
#define IEEE80211_FC0_TYPE_MGT      0x00
#define IEEE80211_FC0_TYPE_CTL      0x04
#define IEEE80211_FC0_TYPE_DATA     0x08

#define IEEE80211_FC0_SUBTYPE_MASK  0xf0
#define IEEE80211_FC0_SUBTYPE_SHIFT 4
/* for TYPE_MGT */
#define IEEE80211_FC0_SUBTYPE_ASSOC_REQ    0x00
#define IEEE80211_FC0_SUBTYPE_ASSOC_RESP   0x10
#define IEEE80211_FC0_SUBTYPE_REASSOC_REQ  0x20
#define IEEE80211_FC0_SUBTYPE_REASSOC_RESP 0x30
#define IEEE80211_FC0_SUBTYPE_PROBE_REQ    0x40
#define IEEE80211_FC0_SUBTYPE_PROBE_RESP   0x50
#define IEEE80211_FC0_SUBTYPE_BEACON       0x80
#define IEEE80211_FC0_SUBTYPE_ATIM         0x90
#define IEEE80211_FC0_SUBTYPE_DISASSOC     0xa0
#define IEEE80211_FC0_SUBTYPE_AUTH         0xb0
#define IEEE80211_FC0_SUBTYPE_DEAUTH       0xc0
#define IEEE80211_FC0_SUBTYPE_ACTION       0xd0
/* for TYPE_CTL */
#define IEEE80211_FC0_BLOCK_ACK_REQ      0x80
#define IEEE80211_FC0_BLOCK_ACK          0x90
#define IEEE80211_FC0_SUBTYPE_PS_POLL    0xa0
#define IEEE80211_FC0_SUBTYPE_RTS        0xb0
#define IEEE80211_FC0_SUBTYPE_CTS        0xc0
#define IEEE80211_FC0_SUBTYPE_ACK        0xd0
#define IEEE80211_FC0_SUBTYPE_CF_END     0xe0
#define IEEE80211_FC0_SUBTYPE_CF_END_ACK 0xf0
/* for TYPE_DATA (bit combination) */
#define IEEE80211_FC0_SUBTYPE_DATA          0x00
#define IEEE80211_FC0_SUBTYPE_CF_ACK        0x10
#define IEEE80211_FC0_SUBTYPE_CF_POLL       0x20
#define IEEE80211_FC0_SUBTYPE_CF_ACPL       0x30
#define IEEE80211_FC0_SUBTYPE_NODATA        0x40
#define IEEE80211_FC0_SUBTYPE_CFACK         0x50
#define IEEE80211_FC0_SUBTYPE_CFPOLL        0x60
#define IEEE80211_FC0_SUBTYPE_CF_ACK_CF_ACK 0x70
#define IEEE80211_FC0_SUBTYPE_QOS           0x80
#define IEEE80211_FC0_SUBTYPE_QOS_NULL      0xc0

#define IEEE80211_FC1_DIR_MASK   0x03
#define IEEE80211_FC1_DIR_NODS   0x00 /* STA->STA */
#define IEEE80211_FC1_DIR_TODS   0x01 /* STA->AP  */
#define IEEE80211_FC1_DIR_FROMDS 0x02 /* AP ->STA */
#define IEEE80211_FC1_DIR_DSTODS 0x03 /* AP ->AP  */

#define IEEE80211_FC1_MORE_FRAG   0x04
#define IEEE80211_FC1_RETRY       0x08
#define IEEE80211_FC1_PWR_MGT     0x10
#define IEEE80211_FC1_MORE_DATA   0x20
#define IEEE80211_FC1_PROTECT_FRM 0x40
#define IEEE80211_FC1_ORDER       0x80

#define IEEE80211_SEQ_FRAG_MASK  0x000f
#define IEEE80211_SEQ_FRAG_SHIFT 0
#define IEEE80211_SEQ_SEQ_MASK   0xfff0
#define IEEE80211_SEQ_SEQ_SHIFT  4

#define IEEE80211_NWID_LEN 32

#define IEEE80211_QOS_TXOP        0x00ff
#define IEEE80211_QOS_AMSDU_S     0x7
#define IEEE80211_QOS_AMSDU       0x1
#define IEEE80211_QOS_ACKPOLICY   0x60
#define IEEE80211_QOS_ACKPOLICY_S 5
#define IEEE80211_QOS_EOSP        0x10
#define IEEE80211_QOS_EOSP_S      4
#define IEEE80211_QOS_TID         0x0f

#define IEEE80211_QOSINFO_COUNT_M        0x0F /* Mask for Param Set Count field */
#define IEEE80211_QOSINFO_UAPSD_CAP      0x80 /* U-APSD bit field */
#define IEEE80211_QOSINFO_VO_APSD_FLAG_S 0    /* U-APSD flag for AC_VO */
#define IEEE80211_QOSINFO_VI_APSD_FLAG_S 1    /* U-APSD flag for AC_VI */
#define IEEE80211_QOSINFO_BK_APSD_FLAG_S 2    /* U-APSD flag for AC_BK */
#define IEEE80211_QOSINFO_BE_APSD_FLAG_S 3    /* U-APSD flag for AC_BE */
#define IEEE80211_QOSINFO_UAPSD_FLAGS_M  0x0F /* U-APSD flag for Sta */
#define IEEE80211_QOSINFO_MAXSP_M        0x3  /* Mask for Max SP length field */
#define IEEE80211_QOSINFO_MAXSP_S        5    /* Shift offset for Max SP Length field */

#define IEEE80211_AMSDU_HEADER_SIZE 14

/*
 * WMM/802.11e information element.
 */
struct ieee80211_ie_wmm_info {
    uint8_t elementId;
    uint8_t len;
    uint8_t wmm_oui[3];  /* 0x00, 0x50, 0xf2 */
    uint8_t wmm_type;    /* OUI type */
    uint8_t wmm_subtype; /* OUI subtype */
    uint8_t wmm_version; /* spec revision */
    uint8_t qos_info;    /* QoS info */
} __ATTRIB_PACK;

/*
 * 802.11e QBSS Information element.
 */
struct ieee80211_qbss_ie {
    uint8_t qbss_id;
    uint8_t qbss_len;
    uint16_t qbss_stationCount;
    uint8_t qbss_channelUtilization;
    uint16_t qbss_aac;
} __ATTRIB_PACK;

/*
 * WMM/802.11e Tspec Element
 */
typedef struct wmm_tspec_ie_t {
    uint8_t elementId;
    uint8_t len;
    uint8_t oui[3];
    uint8_t ouiType;
    uint8_t ouiSubType;
    uint8_t version;
    uint16_t tsInfo_info;
    uint8_t tsInfo_reserved;
    uint16_t nominalMSDU;
    uint16_t maxMSDU;
    uint32_t minServiceInt;
    uint32_t maxServiceInt;
    uint32_t inactivityInt;
    uint32_t suspensionInt;
    uint32_t serviceStartTime;
    uint32_t minDataRate;
    uint32_t meanDataRate;
    uint32_t peakDataRate;
    uint32_t maxBurstSize;
    uint32_t delayBound;
    uint32_t minPhyRate;
    uint16_t sba;
    uint16_t mediumTime;
} __ATTRIB_PACK WMM_TSPEC_IE;

enum ACTION_CATEGORY {
    ACTION_CATEGORY_CODE_SPECMGMT = 0,
    ACTION_CATEGORY_CODE_QOS = 1,
    ACTION_CATEGORY_CODE_DLS = 2,
    ACTION_CATEGORY_CODE_BLOCK_ACK = 3,
    ACTION_CATEGORY_CODE_PUBLIC = 4,
    ACTION_CATEGORY_CODE_RADIO_MEASUREMENT = 5,
    ACTION_CATEGORY_CODE_HT = 7,
    ACTION_CATEGORY_CODE_SA_QUERY = 8,
    ACTION_CATEGORY_CODE_WNM = 10,
    ACTION_CATEGORY_CODE_FINE_TIME_MSMT_REQUEST = 10,
    ACTION_CATEGORY_CODE_WNM_UNPROTECTED = 11,
    ACTION_CATEGORY_CODE_FINE_TIME_MSMT = 11,
    ACTION_CATEGORY_CODE_TSPEC = 17,
    ACTION_CATEGORY_CODE_WUR = 32,
    ACTION_CATEGORY_CODE_VENDOR = 127,
    ACTION_CATEGORY_CODE_TWT = 22,  // 126,
};

enum ACTION_FRAME_FORMAT_SPEC_MGMT {
    ACTION_CODE_MEASUREMENT_REQUEST = 0,
    ACTION_CODE_MEASUREMENT_REPORT = 1,
    ACTION_CODE_TPC_REQUEST = 2,
    ACTION_CODE_TPC_REPORT = 3,
    ACTION_CODE_CHANNEL_SWITCH_ANNOUNCEMENT = 4,
};

enum ACTION_FRAME_FORMAT_TSPECS {
    ACTION_CODE_TSPEC_ADDTS = 0,
    ACTION_CODE_TSPEC_ADDTS_RESP = 1,
    ACTION_CODE_TSPEC_DELTS = 2,
};

enum ACTION_FRAME_FORMAT_RM {
    ACTION_CODE_RADIO_MEASUREMENT_REQUEST = 0,
    ACTION_CODE_RADIO_MEASUREMENT_REPORT = 1,
    ACTION_CODE_LINK_MEASUREMENT_REQUEST = 2,
    ACTION_CODE_LINK_MEASUREMENT_REPORT = 3,
    ACTION_CODE_NEIGHBOR_REPORT_REQUEST = 4,
    ACTION_CODE_NEIGHBOR_REPORT_RESPONSE = 5,
    ACTION_CODE_MEASUREMENT_PILOT = 6,
};

enum ACTION_FRAME_FORMAT_PUBLIC {
    ACTION_CODE_PUBLIC_ECSA = 4,
    ACTION_CODE_PUBLIC_VENDOR_SPECIFIC = 9,
    ACTION_CODE_PUBLIC_GAS_INITIAL_REQUEST = 10,
    ACTION_CODE_PUBLIC_GAS_INITIAL_RESPONSE = 11,
    ACTION_CODE_PUBLIC_GAS_COMEBACK_REQUEST = 12,
    ACTION_CODE_PUBLIC_GAS_COMEBACK_RESPONSE = 13,
};

#ifdef NT_FN_FTM  // For FTM
enum ACTION_FRAME_FORMAT_RTT {
    ACTION_CODE_RTT_FINE_TIME_MSMT_REQUEST = 32,
    ACTION_CODE_RTT_FINE_TIME_MSMT = 33,
};
#endif  // NT_FN_FTM

enum ACTION_FRAME_FORMAT_SA_QUERY {
    ACTION_SA_QUERY_REQUEST = 0,
    ACTION_SA_QUERY_RESPONSE = 1,
};

enum ACTION_FRAME_FORMAT_TWT_QUERY {
    ACTION_TWT_QUERY_REQUEST = 0,
    ACTION_TWT_QUERY_RESPONSE = 1,
};

typedef enum _ACTION_FRAME_FORMAT_UNPROTECTED_S1G {
    ACTION_CODE_S1G_AID_SWITCH_REQUEST = 0,
    ACTION_CODE_S1G_AID_SWITCH_RESPONSE = 1,
    ACTION_CODE_S1G_SYNC_CONTROL = 2,
    ACTION_CODE_S1G_STA_INFORMATION_ANNOUNCEMENT = 3,
    ACTION_CODE_S1G_EDCA_PARAMETER_SET = 4,
    ACTION_CODE_S1G_EL_OPERATION = 5,
    ACTION_CODE_S1G_TWT_SETUP = 6,
    ACTION_CODE_S1G_TWT_TEARDOWN = 7,
    ACTION_CODE_S1G_SECTORIZED_GROUP_ID_LIST = 8,
    ACTION_CODE_S1G_SECTOR_ID_FEEDBACK = 9,
    ACTION_CODE_S1G_RESERVED = 10,
    ACTION_CODE_S1G_TWT_INFORMATION = 11,
} ACTION_FRAME_FORMAT_UNPROTECTED_S1G;

enum ACTION_FRAME_FORMAT_WNM {
    ACTION_WNM_EVENT_REQUEST = 0,
    ACTION_WNM_EVENT_RESPONSE = 1,
    ACTION_WNM_DIAG_REQUEST = 2,
    ACTION_WNM_DIAG_RESPONSE = 3,
    ACTION_WNM_LOCATION_REQUEST = 4,
    ACTION_WNM_LOCATION_RESPONSE = 5,
    ACTION_WNM_BSS_TRANSTION_QUERY = 6,
    ACTION_WNM_BSS_TRANSITION_REQ = 7,
    ACTION_WNM_BSS_TRANSITION_RESP = 8,
    ACTION_WNM_FMS_REQUEST = 9,
    ACTION_WNM_FMS_RESPONSE = 10,
    ACTION_WNM_COLLOCATED_INTF_REQUEST = 11,
    ACTION_WNM_COLLOCATED_INTF_REPORT = 12,
    ACTION_WNM_TFS_REQUEST = 13,
    ACTION_WNM_TFS_RESPONSE = 14,
    ACTION_WNM_TFS_NOTIFY = 15,
    ACTION_WNM_SLEEP_REQUEST = 16,
    ACTION_WNM_SLEEP_RESPONSE = 17,
    ACTION_WNM_TIM_BROADCAST_REQ = 18,
    ACTION_WNM_TIM_BROADCAST_RESP = 19,
    ACTION_WNM_QOS_CAP_UPDATE = 20,
    ACTION_WNM_CHANNEL_USAGE_REQUEST = 21,
    ACTION_WNM_CHANNEL_USAGE_RESPONSE = 22,
    ACTION_WNM_DMS_REQUEST = 23,
    ACTION_WNM_DMS_REPORT = 24,
    ACTION_WNM_TIMING_REQUEST = 25,
    ACTION_WNM_NOTIFICATION_REQUEST = 26,
    ACTION_WNM_NOTIFICATION_RESPONSE = 27,
};

enum ACTION_FRAME_FORMAT_WNM_UNPROTECTED {
    ACTION_WNM_TIM_BROADCAST = 0,
    ACTION_WNM_TIMING_MEASUREMENT = 1,
};

/*
 *TLVs for Vendor IE
 */
enum QCN_IE_TLV {
    TLV_VERSION_ATTR_ID = 0x1,
    TLV_VHT_MCS_10_11_ATTR_ID = 0x2,
    TLV_HE_400NS_SGI_SUPP_ATTR_ID = 0x3,
    TLV_HE_2XLTF_160_80P80_SUPP_ATTR_ID = 0x4,
    TLV_HE_DL_OFDMA_SUPPP_ATTR_ID = 0x5,
    TLV_TRANSITION_REASONP_ATTR_ID = 0x6,
    TLV_TRANSITION_REJECTIONP_ATTR_ID = 0x7,
    TLV_HE_DL_MUMIMO_SUPPP_ATTR_ID = 0x8,
    TLV_HE_MCS_11_12_ATTR_ID = 0x9,
    TLV_REPEATER_INFO_ATTR_ID = 0xa,
    TLV_HE_240_MHZ_SUPP_ATTR_ID = 0Xb,
    TLV_ECSA_ATTR_ID = 0xc,
    TLV_EDCA_PIFS_PARAM_ATTR_ID = 0xd,
    TLV_ATTRIB_MAX_ATTR_ID = 0xe,
};

#define WMM_TSPEC_INFO_LEN 61

#define TSPEC_USER_PRIORITY_MASK 0x7
#define TSPEC_USER_PRIORITY_S    11

#define TSPEC_PS_MASK 0x1
#define TSPEC_PS_S    10

#define TSPEC_ACCESS_POLICY_MASK 0x3
#define TSPEC_ACCESS_POLICY_S    7

#define TSPEC_DIRECTION_MASK 0x3
#define TSPEC_DIRECTION_S    5

#define TSPEC_TSID_MASK 0xF
#define TSPEC_TSID_S    1

#define TSPEC_TRAFFIC_TYPE_MASK 0x1
#define TSPEC_TRAFFIC_TYPE_S    0

#define IEEE80211_ACPARAM_ACI_M      0x60 /* Mask for ACI field */
#define IEEE80211_ACPARAM_ACI_S      5    /* Shift for ACI field */
#define IEEE80211_ACPARAM_ACM_M      0x10 /* Mask for ACM bit */
#define IEEE80211_ACPARAM_ACM_S      4    /* Shift for ACM bit */
#define IEEE80211_ACPARAM_AIFSN_M    0x0f /* Mask for aifsn field */
#define IEEE80211_ACPARAM_LOGCWMIN_M 0x0f /* Mask for CwMin field (in log) */
#define IEEE80211_ACPARAM_LOGCWMAX_M 0xf0 /* Mask for CwMax field (in log) */
#define IEEE80211_ACPARAM_LOGCWMAX_S 4    /* Shift for CwMax field */

/*
 * WMM AC parameter field
 */

struct ieee80211_wmm_acparams {
    uint8_t acp_aci_aifsn;
    uint8_t acp_logcwminmax;
    uint16_t acp_txop;
} __ATTRIB_PACK;

/*
 * WMM Parameter Element
 */

struct ieee80211_ie_wmm_param {
    uint8_t elementId;
    uint8_t len;
    uint8_t param_oui[3];
    uint8_t param_oui_type;
    uint8_t param_oui_sybtype;
    uint8_t param_version;
    uint8_t param_qosInfo;
    uint8_t param_reserved;
    struct ieee80211_wmm_acparams params_acParams[WMM_NUM_AC];
} __ATTRIB_PACK;

/*
 * 802.11i/WPA information element (maximally sized).
 */
struct ieee80211_ie_wpa {
    uint8_t wpa_id;           /* IEEE80211_ELEMID_VENDOR */
    uint8_t wpa_len;          /* length in bytes */
    uint8_t wpa_oui[3];       /* 0x00, 0x50, 0xf2 */
    uint8_t wpa_type;         /* OUI type */
    uint16_t wpa_version;     /* spec revision */
    uint32_t wpa_mcipher[1];  /* multicast/group key cipher */
    uint16_t wpa_uciphercnt;  /* # pairwise key ciphers */
    uint32_t wpa_uciphers[8]; /* ciphers */
    uint16_t wpa_authselcnt;  /* authentication selector cnt*/
    uint32_t wpa_authsels[8]; /* selectors */
    uint16_t wpa_caps;        /* 802.11i capabilities */
    uint16_t wpa_pmkidcnt;    /* 802.11i pmkid count */
    uint16_t wpa_pmkids[8];   /* 802.11i pmkids */
} __ATTRIB_PACK;

/*
 * Management information element payloads.
 */

enum {
    IEEE80211_ELEMID_SSID = 0,
    IEEE80211_ELEMID_RATES = 1,
    IEEE80211_ELEMID_FHPARMS = 2,
    IEEE80211_ELEMID_DSPARMS = 3,
    IEEE80211_ELEMID_CFPARMS = 4,
    IEEE80211_ELEMID_TIM = 5,
    IEEE80211_ELEMID_IBSSPARMS = 6,
    IEEE80211_ELEMID_COUNTRY = 7,
    IEEE80211_ELEMID_QBSS = 11,
    IEEE80211_ELEMID_CHALLENGE = 16,
    /* 17-31 reserved for challenge text extension */
    IEEE80211_ELEMID_PWRCNSTR = 32,
    IEEE80211_ELEMID_PWRCAP = 33,
    IEEE80211_ELEMID_TPCREQ = 34,
    IEEE80211_ELEMID_TPCREP = 35,
    IEEE80211_ELEMID_SUPPCHAN = 36,
    IEEE80211_ELEMID_CHANSWITCH = 37,
    IEEE80211_ELEMID_MEASREQ = 38,
    IEEE80211_ELEMID_MEASREP = 39,
    IEEE80211_ELEMID_QUIET = 40,
    IEEE80211_ELEMID_IBSSDFS = 41,
    IEEE80211_ELEMID_ERP = 42,
    IEEE80211_ELEMID_HTCAP_ANA = 45, /* Address ANA, and non-ANA story, for interop. CL#171733 */
    IEEE80211_ELEMID_RSN = 48,
    IEEE80211_ELEMID_XRATES = 50,
    IEEE80211_ELEMID_TIMEOUTINTRVL = 56, /*802.11w Timeout Interval ID */
    IEEE80211_ELEMID_APCHANREP = 51,
    IEEE80211_ELEMID_NEIGHBORREP = 52,
    IEEE80211_ELEMID_EXTCHANSWITCH = 60,
    IEEE80211_ELEMID_HTINFO_ANA = 61,
    IEEE80211_ELEMID_SEC_CHANID = 62,
    IEEE80211_ELEMID_WAPI = 68,
    IEEE80211_ELEMID_2040_BSS_COEX = 72,
    IEEE80211_ELEMID_MMIE = 76, /* 802.11w Management MIC IE */
    IEEE80211_ELEMID_BSS_MAX_IDLE = 90,
    IEEE80211_TFS_REQUEST = 91,
    IEEE80211_TFS_RESPONSE = 92,
    IEEE80211_WNM_SLEEP = 93,
    IEEE80211_TIM_BROADCAST_REQ = 94,
    IEEE80211_TIM_BROADCAST_RESP = 95,
    IEEE80211_ELEMID_ADV_PROTO = 108,
    IEEE80211_ELEMID_EXT_CAP = 127,
    IEEE80211_ELEMID_TPC = 150,
    IEEE80211_ELEMID_CCKM = 156,
#ifdef NT_FN_FTM
    IEEE80211_ELEMID_FTM = 206,
#endif                             // NT_FN_FTM
    IEEE80211_ELEMID_TWT = 216,    /* 802.11ax TWT IE */
    IEEE80211_ELEMID_VENDOR = 221, /* vendor private */
    IEEE80211_ELEMID_RSNXE = 244,
    IEEE80211BA_ELEMID_EXTENSION = 255
};

/*Element ID Extension (EID 255) values*/
enum {
    IEEE80211_ELEMID_EXT_PASSWORD_IDENTIFIER = 33,
    IEEE80211_ELEMID_EXT_HE_CAP_ELEMENT = 35,
    IEEE80211_ELEMID_EXT_HE_OP_ELEMENT = 36,
    IEEE80211_ELEMID_EXT_ANTI_CLOGGING_ELEMENT = 93
};

#define IEEE80211_SSID_MAXLEN           32
#define IEEE80211_MIN_COUNTRY_IE_LENGTH 3

typedef struct {
    uint8_t elemId;
    uint8_t length;
    uint8_t txpower;
    uint8_t linkMargin;
} __ATTRIB_PACK tpc_report_t;

typedef struct {
    uint8_t elemId;
    uint8_t length;
} __ATTRIB_PACK tpc_request_t;

struct ieee80211_tim_ie {
    uint8_t tim_ie; /* IEEE80211_ELEMID_TIM */
    uint8_t tim_len;
    uint8_t dtim_count;    /* DTIM count */
    uint8_t dtim_period;   /* DTIM period */
    uint8_t tim_bitctl;    /* bitmap control */
    uint8_t tim_bitmap[1]; /* variable-length bitmap */
} __ATTRIB_PACK;

#define IEEE80211_COUNTRYIE_MIN_LEN  6
#define IEEE80211_COUNTRYIE_BAND_MAX 4

struct ieee80211_country_ie {
    uint8_t ie; /* IEEE80211_ELEMID_COUNTRY */
    uint8_t len;
    uint8_t cc[3]; /* ISO CC+(I)ndoor/(O)utdoor */
    struct country_ie_band {
        uint8_t schan;    /* starting channel */
        uint8_t nchan;    /* number channels */
        uint8_t maxtxpwr; /* tx power cap */
    } __ATTRIB_PACK band[IEEE80211_COUNTRYIE_BAND_MAX];
} __ATTRIB_PACK;

#define IEEE80211_SUB_BAND_MAX 20
struct ieee80211_powercaps_ie {
    uint8_t ie;
    uint8_t len;
    int8_t minPwr;
    int8_t maxPwr;
} __ATTRIB_PACK;

struct ieee80211_suppchan_ie {
    uint8_t ie;
    uint8_t len;
    struct subband_ {
        uint8_t schan;
        uint8_t nchan;
    } __ATTRIB_PACK subband[IEEE80211_SUB_BAND_MAX];
} __ATTRIB_PACK;

typedef struct ap_chan_report_s {
    uint8_t elemId;
    uint8_t length;
    uint8_t regClass;
    uint8_t clist[1]; /* variable length */
} __ATTRIB_PACK ap_chan_report_t;

typedef struct neighbor_report_s {
    uint8_t elemId;
    uint8_t length;
    uint8_t bssid[IEEE80211_ADDR_LEN];
    uint32_t bssidInfo;
    uint8_t regClass;
    uint8_t channel;
    uint8_t phyType;
    uint8_t extension[1]; /* variable length */
} __ATTRIB_PACK neighbor_report_t;

#define IEEE80211_CHALLENGE_LEN 128

#define IEEE80211_TPCREP_LEN       4
#define IEEE80211_PWR_CONSTRNT_LEN 1
#define IEEE80211_CSA_IE_LEN       3
#define IEEE80211_RADAR_11HCOUNT   5
struct ieee80211_csa_ie {
    uint8_t chswitch_mode;
    uint8_t new_ch;
    uint8_t chswitch_count;
} __ATTRIB_PACK;

struct ieee80211_ecsa_ie {
    uint8_t chswitch_mode;
    uint8_t new_operating_class;
    uint8_t new_ch;
    uint8_t chswitch_count;
} __ATTRIB_PACK;

#define IEEE80211_RATE_BASIC 0x80
#define IEEE80211_RATE_VAL   0x7f

/* EPR information element flags */
#define IEEE80211_ERP_NON_ERP_PRESENT 0x01
#define IEEE80211_ERP_USE_PROTECTION  0x02
#define IEEE80211_ERP_LONG_PREAMBLE   0x04

/*
 * Management Action Frames
 */

/* generic frame format */
struct ieee80211_action {
    uint8_t ia_category;
    uint8_t ia_action;
} __ATTRIB_PACK;

/* categories */
#define IEEE80211_ACTION_CAT_QOS 0 /* qos */
#define IEEE80211_ACTION_CAT_BA  3 /* BA */
#define IEEE80211_ACTION_CAT_HT  7 /* HT per IEEE802.11n-D1.06 */

/* HT actions */
#define IEEE80211_ACTION_HT_TXCHWIDTH   0 /* recommended transmission channel width */
#define IEEE80211_ACTION_HT_SMPOWERSAVE 1 /* Spatial Multiplexing (SM) Power Save */

/* HT - recommended transmission channel width */
struct ieee80211_action_ht_txchwidth {
    struct ieee80211_action at_header;
    uint8_t at_chwidth;
} __ATTRIB_PACK;

#define IEEE80211_A_HT_TXCHWIDTH_20   0
#define IEEE80211_A_HT_TXCHWIDTH_2040 1

/* HT - Spatial Multiplexing (SM) Power Save */
struct ieee80211_action_ht_smpowersave {
    struct ieee80211_action as_header;
    uint8_t as_control;
} __ATTRIB_PACK;

/* values defined for 'as_control' field per 802.11n-D1.06 */
#define IEEE80211_A_HT_SMPOWERSAVE_DISABLED 0x00 /* SM Power Save Disabled, SM packets ok  */
#define IEEE80211_A_HT_SMPOWERSAVE_ENABLED  0x01 /* SM Power Save Enabled bit  */
#define IEEE80211_A_HT_SMPOWERSAVE_MODE     0x02 /* SM Power Save Mode bit */
#define IEEE80211_A_HT_SMPOWERSAVE_RESERVED 0xFC /* SM Power Save Reserved bits */

/* values defined for SM Power Save Mode bit */
#define IEEE80211_A_HT_SMPOWERSAVE_STATIC  0x00 /* Static, SM packets not ok */
#define IEEE80211_A_HT_SMPOWERSAVE_DYNAMIC 0x02 /* Dynamic, SM packets ok if preceded by RTS */

/* BA actions */
#define IEEE80211_ACTION_BA_ADDBA_REQUEST  0 /* ADDBA request */
#define IEEE80211_ACTION_BA_ADDBA_RESPONSE 1 /* ADDBA response */
#define IEEE80211_ACTION_BA_DELBA          2 /* DELBA */

struct ieee80211_ba_parameterset {
    uint16_t amsdusupported : 1, /* B0   amsdu supported */
        bapolicy : 1,            /* B1   block ack policy */
        tid : 4,                 /* B2-5   TID */
        buffersize : 10;         /* B6-15  buffer size */
} __ATTRIB_PACK;

#define IEEE80211_BA_POLICY_DELAYED   0
#define IEEE80211_BA_POLICY_IMMEDIATE 1
#define IEEE80211_BA_AMSDU_SUPPORTED  1

struct ieee80211_ba_seqctrl {
    uint16_t fragnum : 4, /* B0-3  fragment number */
        startseqnum : 12; /* B4-15  starting sequence number */
} __ATTRIB_PACK;

struct ieee80211_delba_parameterset {
    uint16_t reserved0 : 11, /* B0-10   reserved */
        initiator : 1,       /* B11     initiator */
        tid : 4;             /* B12-15  tid */
} __ATTRIB_PACK;

/* BA - ADDBA request */
struct ieee80211_action_ba_addbarequest {
    struct ieee80211_action rq_header;
    uint8_t rq_dialogtoken;
    struct ieee80211_ba_parameterset rq_baparamset;
    uint16_t rq_batimeout; /* in TUs */
    struct ieee80211_ba_seqctrl rq_basequencectrl;
} __ATTRIB_PACK;

/* BA - ADDBA response */
struct ieee80211_action_ba_addbaresponse {
    struct ieee80211_action rs_header;
    uint8_t rs_dialogtoken;
    uint16_t rs_statuscode;
    struct ieee80211_ba_parameterset rs_baparamset;
    uint16_t rs_batimeout; /* in TUs */
} __ATTRIB_PACK;

/* BA - DELBA */
struct ieee80211_action_ba_delba {
    struct ieee80211_action dl_header;
    struct ieee80211_delba_parameterset dl_delbaparamset;
    uint16_t dl_reasoncode;
} __ATTRIB_PACK;

/*
 * BAR frame format
 */
#define IEEE80211_BAR_CTL_TID_M 0xF000 /* tid mask             */
#define IEEE80211_BAR_CTL_TID_S 12     /* tid shift            */
#define IEEE80211_BAR_CTL_NOACK 0x0001 /* no-ack policy        */
#define IEEE80211_BAR_CTL_COMBA 0x0004 /* compressed block-ack */
struct ieee80211_frame_bar {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_ra[IEEE80211_ADDR_LEN];
    uint8_t i_ta[IEEE80211_ADDR_LEN];
    uint16_t i_ctl;
    uint16_t i_seq;
    /* FCS */
} __ATTRIB_PACK;

/*
 * SA Query Action mgmt Frame
 */
struct ieee80211_action_sa_query {
    struct ieee80211_action sa_header;
    uint16_t sa_transId;
} __ATTRIB_PACK;

#define IEEE80211_MCS_OFFSET        0x5
#define IEEE80211_NUM_MCS_PER_CHAIN 0x8
struct ieee80211_ie_htcap_cmn {
    uint16_t hc_cap;         /* HT capabilities */
    uint8_t hc_maxampdu : 2, /* B0-1 maximum rx A-MPDU factor */
        hc_mpdudensity : 3,  /* B2-4 MPDU density (aka Minimum MPDU Start Spacing) */
        hc_reserved : 3;     /* B5-7 reserved */
    uint8_t hc_mcsset[16];   /* supported MCS set */
    uint16_t hc_extcap;      /* extended HT capabilities */
    uint32_t hc_txbf;        /* txbf capabilities */
    uint8_t hc_antenna;      /* antenna capabilities */
} __ATTRIB_PACK;

/*
 * 802.11n HT Capability IE
 */
struct ieee80211_ie_htcap {
    uint8_t hc_id;  /* element ID */
    uint8_t hc_len; /* length in bytes */
    struct ieee80211_ie_htcap_cmn hc_ie;
} __ATTRIB_PACK;

/* 802.11v WNM Sleep Mode IE */
struct ieee80211_ie_wnm_sleep {
    uint8_t wnmSleep_id;         /* element ID */
    uint8_t wnmSleep_len;        /* length in bytes */
    uint8_t wnmSleep_action;     /*Action type 0- Enter sleep; 1- Exit sleep*/
    uint8_t wnmSleep_respStatus; /* Response status */
    uint16_t wnmSleep_interval;  /* number of DTIM intervals*/

} __ATTRIB_PACK;

/* 802.11v WNM BSS max idle period IE */
struct ieee80211_ie_wnm_max_idle_period {
    uint8_t wnmMaxIdle_id;      /* element ID */
    uint8_t wnmMaxIdle_len;     /* length in bytes */
    uint16_t wnmMaxIdle_period; /* period in 1000 TUs */
    uint8_t wnmMaxIdle_options; /* idle options */
} __ATTRIB_PACK;

/*WNM Sleep mode action type*/
#define IEEE80211_WNM_ENTER_SLEEP 0
#define IEEE80211_WNM_EXIT_SLEEP  1

/*WNM sleep mode Response status*/
#define IEEE80211_WNM_SLEEP_ENTEREXIT_ACCEPT 0
#define IEEE80211_WNM_SLEEP_EXIT_ACCEPT_GTK  1
#define IEEE80211_WNM_SLEEP_DENIED           2
#define IEEE80211_WNM_SLEEP_DENIED_TEMP      3
#define IEEE80211_WNM_SLEEP_DENIED_KEY       4
#define IEEE80211_WNM_SLEEP_DENIED_BUSY      5

/* HT capability flags */
#define IEEE80211_HTCAP_C_ADVCODING           0x0001
#define IEEE80211_HTCAP_C_CHWIDTH40           0x0002
#define IEEE80211_HTCAP_C_SMPOWERSAVE_STATIC  0x0000 /* Capable of SM Power Save (Static) */
#define IEEE80211_HTCAP_C_SMPOWERSAVE_DYNAMIC 0x0004 /* Capable of SM Power Save (Dynamic) */
#define IEEE80211_HTCAP_C_SM_RESERVED         0x0008 /* Reserved */
#define IEEE80211_HTCAP_C_SM_ENABLED          0x000c /* SM enabled, no SM Power Save */
#define IEEE80211_HTCAP_C_GREENFIELD          0x0010
#define IEEE80211_HTCAP_C_SHORTGI20           0x0020
#define IEEE80211_HTCAP_C_SHORTGI40           0x0040
#define IEEE80211_HTCAP_C_TXSTBC              0x0080
#define IEEE80211_HTCAP_C_RXSTBC              0x0300 /* 2 bits */
#define IEEE80211_HTCAP_C_RXSTBC_S            8
#define IEEE80211_HTCAP_C_DELAYEDBLKACK       0x0400
#define IEEE80211_HTCAP_C_MAXAMSDUSIZE        0x0800 /* 1 = 8K, 0 = 3839B */
#define IEEE80211_HTCAP_C_DSSSCCK40           0x1000
#define IEEE80211_HTCAP_C_PSMP                0x2000
#define IEEE80211_HTCAP_C_INTOLERANT40        0x4000
#define IEEE80211_HTCAP_C_LSIGTXOPPROT        0x8000

#define IEEE80211_HTCAP_C_SM_MASK 0x000c /* Spatial Multiplexing (SM) capabitlity bitmask */

/* B0-1 maximum rx A-MPDU factor 2^(13+Max Rx A-MPDU Factor) */
enum {
    IEEE80211_HTCAP_MAXRXAMPDU_8192,  /* 2 ^ 13 */
    IEEE80211_HTCAP_MAXRXAMPDU_16384, /* 2 ^ 14 */
    IEEE80211_HTCAP_MAXRXAMPDU_32768, /* 2 ^ 15 */
    IEEE80211_HTCAP_MAXRXAMPDU_65536, /* 2 ^ 16 */
};
#define IEEE80211_HTCAP_MAXRXAMPDU_FACTOR 13

/* B2-4 MPDU density (usec) */
enum {
    IEEE80211_HTCAP_MPDUDENSITY_NA,   /* No time restriction */
    IEEE80211_HTCAP_MPDUDENSITY_0_25, /* 1/4 usec */
    IEEE80211_HTCAP_MPDUDENSITY_0_5,  /* 1/2 usec */
    IEEE80211_HTCAP_MPDUDENSITY_1,    /* 1 usec */
    IEEE80211_HTCAP_MPDUDENSITY_2,    /* 2 usec */
    IEEE80211_HTCAP_MPDUDENSITY_4,    /* 4 usec */
    IEEE80211_HTCAP_MPDUDENSITY_8,    /* 8 usec */
    IEEE80211_HTCAP_MPDUDENSITY_16,   /* 16 usec */
};

/* HT extended capability flags */
#define IEEE80211_HTCAP_EXTC_PCO                0x0001
#define IEEE80211_HTCAP_EXTC_TRANS_TIME_RSVD    0x0000
#define IEEE80211_HTCAP_EXTC_TRANS_TIME_400     0x0002 /* 20-40 switch time */
#define IEEE80211_HTCAP_EXTC_TRANS_TIME_1500    0x0004 /* in us             */
#define IEEE80211_HTCAP_EXTC_TRANS_TIME_5000    0x0006
#define IEEE80211_HTCAP_EXTC_RSVD_1             0x00f8
#define IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_NONE  0x0000
#define IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_RSVD  0x0100
#define IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_UNSOL 0x0200
#define IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_FULL  0x0300
#define IEEE80211_HTCAP_EXTC_RSVD_2             0xfc00

struct ieee80211_ie_htinfo_cmn {
    uint8_t hi_ctrlchannel;      /* control channel */
    uint8_t hi_extchoff : 2,     /* B0-1 extension channel offset */
        hi_txchwidth : 1,        /* B2   recommended xmiss width set */
        hi_rifsmode : 1,         /* B3   rifs mode */
        hi_ctrlaccess : 1,       /* B4   controlled access only */
        hi_serviceinterval : 3;  /* B5-7 svc interval granularity */
    uint8_t hi_opmode : 2,       /* B0-1  Operating Mode */
        hi_nongfpresent : 1,     /* B2    Non-greenfield STAs present */
        hi_txburstlimit : 1,     /* B3    Transmit Burst Limit */
        hi_obssnonhtpresent : 1, /* B4    OBSS Non-HT STAs Present */
        hi_reserved0 : 3;        /* B5-7 Reserved */
    uint8_t hi_reserved1;
    uint16_t hi_miscflags;

    uint8_t hi_basicmcsset[16]; /* basic MCS set */
} __ATTRIB_PACK;

/*
 * 802.11n HT Information IE
 */
struct ieee80211_ie_htinfo {
    uint8_t hi_id;  /* element ID */
    uint8_t hi_len; /* length in bytes */
    struct ieee80211_ie_htinfo_cmn hi_ie;
} __ATTRIB_PACK;

/* extension channel offset (2 bit signed number) */
enum {
    IEEE80211_HTINFO_EXTOFFSET_NA = 0,    /* 0  no extension channel is present */
    IEEE80211_HTINFO_EXTOFFSET_ABOVE = 1, /* +1 extension channel above control channel */
    IEEE80211_HTINFO_EXTOFFSET_UNDEF = 2, /* -2 undefined */
    IEEE80211_HTINFO_EXTOFFSET_BELOW = 3  /* -1 extension channel below control channel*/
};

/* recommended transmission width set */
enum { IEEE80211_HTINFO_TXWIDTH_20, IEEE80211_HTINFO_TXWIDTH_2040 };

/* operating flags */
#define IEEE80211_HTINFO_OPMODE_PURE           0x00 /* no protection */
#define IEEE80211_HTINFO_OPMODE_MIXED_PROT_OPT 0x01 /* prot optional (legacy device maybe present) */
#define IEEE80211_HTINFO_OPMODE_MIXED_PROT_40  0x02 /* prot required (20 MHz) */
#define IEEE80211_HTINFO_OPMODE_MIXED_PROT_ALL 0x03 /* prot required (legacy devices present) */
#define IEEE80211_HTINFO_OPMODE_MASK           0x03 /* For protection 0x00-0x03 */

/* Non-greenfield STAs present */
enum {
    IEEE80211_HTINFO_NON_GF_NOT_PRESENT, /* Non-greenfield STAs not present */
    IEEE80211_HTINFO_NON_GF_PRESENT,     /* Non-greenfield STAs present */
};

/* Transmit Burst Limit */
enum {
    IEEE80211_HTINFO_TXBURST_UNLIMITED, /* Transmit Burst is unlimited */
    IEEE80211_HTINFO_TXBURST_LIMITED,   /* Transmit Burst is limited */
};

/* OBSS Non-HT STAs present */
enum {
    IEEE80211_HTINFO_OBBSS_NONHT_NOT_PRESENT, /* OBSS Non-HT STAs not present */
    IEEE80211_HTINFO_OBBSS_NONHT_PRESENT,     /* OBSS Non-HT STAs present */
};

/* misc flags */
#define IEEE80211_HTINFO_BASICSTBCMCS    0x007F /* B0-6 basic STBC MCS */
#define IEEE80211_HTINFO_DUALSTBCPROT    0x0080 /* B7   dual stbc protection */
#define IEEE80211_HTINFO_SECONDARYBEACON 0x0100 /* B8   secondary beacon */
#define IEEE80211_HTINFO_LSIGTXOPPROT    0x0200 /* B9   lsig txop prot full support */
#define IEEE80211_HTINFO_PCOACTIVE       0x0400 /* B10  pco active */
#define IEEE80211_HTINFO_PCOPHASE        0x0800 /* B11  pco phase */

/* RIFS mode */
enum {
    IEEE80211_HTINFO_RIFSMODE_PROHIBITED, /* use of rifs prohibited */
    IEEE80211_HTINFO_RIFSMODE_ALLOWED,    /* use of rifs permitted */
};

/* Advertisement Protocol ID definitions (IEEE 802.11u) */
enum ADV_PROTOCOL_ID {
    ADV_PROTOCOL_NATIVE_QUERY_PROTOCOL = 0,
    ADV_PROTOCOL_MIH_INFO_SERVICE = 1,
    ADV_PROTOCOL_MIH_CMD_AND_EVENT_DISCOVERY = 2,
    ADV_PROTOCOL_EMERGENCY_ALERT_SYSTEM = 3,
    ADV_PROTOCOL_LOCATION_TO_SERVICE = 4,
    ADV_PROTOCOL_VENDOR_SPECIFIC = 221
};

/* Native Query Protocol info ID definitions (IEEE 802.11u) */
enum NQP_INFO_ID { NQP_INFO_VENDOR_SPECIFIC = 56797 };

/*
 * 802.11n Extended Capabilities IE
 */
#define IEEE80211_EXTCAP_2040COEX_MGMT 0x01
#define IEEE80211_EXTCAP_PSMP_CAP      0x02

#ifdef NT_FN_FTM
#define IEEE80211_EXTCAP_FTM_RESPONDER 0x46  // 70
#define IEEE80211_EXTCAP_FTM_INITIATOR 0x47  // 71
#endif                                       // NT_FN_FTM

struct ieee80211_ie_ext_cap_filed {
    //	uint8_t reserved_1 : 8;	/*Position_1 to 8*/
    // octet 1
    uint8_t wnm_2040_coex_mgnt : 1; /*Position_1*/
    uint8_t reserved_2 : 1;         /*Position_1*/
    uint8_t reserved_3 : 1;         /*Position_1*/
    uint8_t reserved_4 : 1;         /*Position_1*/
    uint8_t reserved_5 : 1;         /*Position_1*/
    uint8_t reserved_6 : 1;         /*Position_1*/
    uint8_t reserved_7 : 1;         /*Position_1*/
    uint8_t reserved_8 : 1;         /*Position_1*/

    // octet 2
    uint8_t proxy_arp_cap_12 : 1; /*Position_1*/
    uint8_t reserved_10 : 1;      /*Position_1*/
    uint8_t reserved_11 : 1;      /*Position_1*/
    uint8_t reserved_12 : 1;      /*Position_1*/
    uint8_t reserved_13 : 4;      /*Position_13 to 16*/

    // octet 3
    uint8_t wnm_sleep_cap_17 : 1; /*Position_17*/
    uint8_t reserved_18 : 1;      /*Position_18*/
    uint8_t reserved_19 : 1;      /*Position_19*/
    uint8_t reserved_20 : 1;      /*Position_20*/
    uint8_t reserved_21 : 1;      /*Position_21*/
    uint8_t reserved_22 : 1;      /*Position_22*/
    uint8_t reserved_23 : 1;      /*Position_23*/
    uint8_t reserved_24 : 1;      /*Position_24*/

    // octet 4
    uint8_t reserved_25 : 8; /*Position_25 to 32*/

    // octet 5
    uint8_t reserved_33 : 8; /*Position_33 to 40*/

    // octet 6
    uint8_t reserved_41 : 8; /*Position_41 to 48*/

    // octet 7
    uint8_t reserved_49 : 8; /*Position_49 to 56*/

    // octet 8 & 9
    uint8_t reserved_57 : 8;          /*Position_57 to 64*/
                                      //	uint8_t reserved_9 : 8;	/*65 to 72*/
    uint8_t reserved_65 : 1;          /*Position_65*/
    uint8_t reserved_66 : 1;          /*Position_66*/
    uint8_t reserved_67 : 1;          /*Position_67*/
    uint8_t reserved_68 : 1;          /*Position_68*/
    uint8_t reserved_69 : 1;          /*Position_69*/
    uint8_t ftm_responder_xcp_70 : 1; /*Position_70: IEEE80211_EXTCAP_FTM_RESPONDER*/
    uint8_t ftm_initiator_xcp_71 : 1; /*Position_71: IEEE80211_EXTCAP_FTM_INITIATOR*/
    uint8_t reserved_72 : 1;          /*Position_72: Reserved*/

    // octet 10
    uint8_t reserved_73_to_77 : 5; /*Position_73_to_77: Reserved*/
    uint8_t twt_requester : 1;     /*Position_78: twt requester */
    uint8_t twt_respnder : 1;      /*Position_79: twt responder */
    uint8_t reserved_80 : 1;       /*Position_80 : reserved */

    // octet 11
    uint8_t reserved_81 : 8; /* Position_81 to 88 */

    // octet 12
    uint8_t reserved_89 : 1;
    uint8_t twt_parameter_range : 1; /* Position_90: twt parameter range */
    uint8_t reserved_91 : 6;         /*Position_91 to 96 : reserved */
} __ATTRIB_PACK;

struct ieee80211_ie_ext_cap {  // Extended Cap IE
    uint8_t xc_id;             /* element ID */
    uint8_t xc_len;            /* length in bytes */
                               //    uint8_t                        xc_capflags;
    struct ieee80211_ie_ext_cap_filed xc_capflags;
} __ATTRIB_PACK;

#define IEEE80211_EXT_CAP_IE_SIZE sizeof(struct ieee80211_ie_ext_cap_filed)

#endif /* __IEEE80211_DEFS_H__ */
