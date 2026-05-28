
/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _IEEE80211_H_
#define _IEEE80211_H_

#include "ieee80211_defs.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

/* is 802.11 address multicast/broadcast? */
#define IEEE80211_IS_MULTICAST(_a)   (*(_a)&0x01)
#define IEEE80211_IS_BROADCAST(_a)   (*(_a) == 0xFF)
#define IEEE80211_IS_QOSENABLED(_ni) ((_ni) && (_ni->ni_flags & IEEE80211_NODE_QOS))
#define IEEE80211_SET_PWR_BIT(bf)                          \
    {                                                      \
        struct ieee80211_frame *wh;                        \
        wh = (struct ieee80211_frame *)WLAN_BUF_START(bf); \
        wh->i_fc[1] |= IEEE80211_FC1_PWR_MGT;              \
    }
#define WEP_HEADER   (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN)
#define WEP_TRAILER  IEEE80211_WEP_CRCLEN
#define CCMP_HEADER  (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_EXTIVLEN)
#define CCMP_TRAILER IEEE80211_WEP_MICLEN
#define TKIP_HEADER  (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_EXTIVLEN)
#define TKIP_TRAILER IEEE80211_WEP_CRCLEN
#define TKIP_MICLEN  IEEE80211_WEP_MICLEN

/*
 * The number of Adhoc STAs for Dragon is 4
 */
#define IEEE80211_INFRA_AID_MAX 2007

/*
 * Time Unit Related
 */

#define TU_TO_USEC(t) ((t) << 10)
#define USEC_TO_TU(t) ((t) >> 10)

/*
 * IBSS Paramter Length
 */

#define IEEE80211_IBSS_PARAM_LEN 2

/*
 * Channel Access Parameters for the Beacon Q
 */

#define ADHOC_BEACON_Q_AIFS  2
#define ADHOC_BEACON_Q_CWMIN 31
#define ADHOC_BEACON_Q_CWMAX 1023

#define ADHOC_BEACON_Q_CWMIN_11B 31
#define ADHOC_BEACON_Q_CWMAX_11B 1023
#define AP_BEACON_Q_AIFS         1
#define AP_BEACON_Q_CWMIN        0
#define AP_BEACON_Q_CWMAX        0

#define IS_CTL(wh)           ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL)
#define WLAN_CTRL_FRAME_SIZE (2 + 2 + 6 + 4)

#define IS_DATA(wh) ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_DATA)

#define IS_SCAN_FRM(wh)                                                              \
    (((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_BEACON) || \
     ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PROBE_RESP))

#define IS_ACTION_FRM(wh)                                                   \
    (((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_MGT) && \
     ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_ACTION))

#define IEEE80211_MAX_TX_CACHE_FLUSH (IEEE80211_HDR_LEN + LLC_SNAPFRAMELEN)

#define IEEE80211_MAX_RX_CACHE_INVAL (IEEE80211_HDR_LEN + LLC_SNAPFRAMELEN)

/*
 * Trim the stats structure to have only the members which is collected
 */
#ifdef NT_FN_PRODUCTION_STATS
struct ieee80211_stats {
    uint16_t is_rx_badversion;                       /* rx frame with bad version */
    uint16_t is_rx_tooshort;                         /* rx frame too short */
    uint16_t is_rx_wrongbss;                         /* rx from wrong bssid */
    uint16_t is_rx_dup;                              /* rx discard 'cuz dup */
    uint16_t is_rx_wrongdir;                         /* rx w/ wrong direction */
    uint16_t is_rx_mcastecho;                        /* rx discard 'cuz mcast echo */
    uint16_t is_rx_noprivacy;                        /* rx w/ wep but privacy off */
    uint16_t is_rx_mgtdiscard;                       /* rx discard mgt frames */
    uint16_t is_rx_ctl;                              /* rx discard ctrl frames */
    uint16_t is_rx_rstoobig;                         /* rx rate set truncated */
    uint16_t is_rx_elem_missing;                     /* rx required element missing*/
    uint16_t is_rx_elem_toobig;                      /* rx element too big */
    uint16_t is_rx_elem_toosmall;                    /* rx element too small */
    uint16_t is_rx_elem_unknown;                     /* rx element unknown */
    uint16_t is_rx_ssidmismatch;                     /* rx frame ssid mismatch  */
    uint16_t is_rx_auth_unsupported;                 /* rx w/ unsupported auth alg */
    uint16_t is_rx_auth_fail;                        /* rx sta auth failure */
    uint16_t is_rx_auth_countermeasures;             /* rx auth discard 'cuz CM */
    uint16_t is_rx_assoc_norate;                     /* rx assoc w/ no rate match */
    uint16_t is_rx_deauth;                           /* rx deauthentication */
    uint16_t is_rx_disassoc; /* rx disassociation */ /* rx disassociation */
    uint16_t is_rx_badsubtype;                       /* rx frame w/ unknown subtype*/
    uint16_t is_rx_assoc_notauth;                    /* rx assoc w/o auth */
    uint16_t is_rx_bad_auth;                         /* rx bad auth request */
    uint16_t is_aggr_dup;                            /* dups with aggr on */
    uint16_t is_rx_assoc_fail;                       /* rx sta assoc failure */
    uint16_t is_rx_beacon;                           /* sta rx beacon cnt */
    uint16_t is_rx_probe_req;                        /* ap rx probe req cnt */
    uint16_t is_rx_probe_resp;                       /* sta rx probe resp cnt */
    uint16_t is_rx_auth_rcvd;                        /* rx auth frame cnt */
    uint16_t is_rx_assoc_req;                        /* ap rx assoc req cnt */
    uint16_t is_rx_assoc_resp;                       /* sta rx assoc resp cnt */
    uint16_t pre_bcn_intr;                           /* pre beacon interrupt */
    uint16_t ps_poll_tx_pkts;
    uint16_t ps_poll_rx_pkts;
    uint16_t is_tx_deauth;                           /* tx deauthentication */
    uint16_t is_tx_disassoc; /* tx disassociation */ /* tx disassociation */
    uint16_t is_tx_beacon;                           /* AP tx beacon cnt */
    uint16_t is_tx_probe_req;                        /* sta tx probe req cnt */
    uint16_t is_tx_probe_resp;                       /* ap tx probe resp cnt */
    uint16_t is_tx_auth;                             /* tx auth frame cnt */
    uint16_t is_tx_assoc_req;                        /* sta tx assoc req cnt */
    uint16_t is_tx_assoc_resp;                       /* ap tx assoc resp cnt */
    uint16_t is_tx_action_cnt;                       /* tx Action frame */
};
#endif  // NT_FN_PRODUCTION_STATS
/* does frame have QoS sequence control data */
#define IEEE80211_QOS_HAS_SEQ(wh)                                               \
    (((wh)->i_fc[0] & (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) == \
     (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS))

/* Get the tid from frame */
#define IEEE80211_QOS_GET_TID(_x) ((_x)->i_qos[0] & IEEE80211_QOS_TID)

/*
 * Return the Total Length of an 802.11 IE.
 */

#define IEEE80211_IE_LEN(pIe) (((uint8_t *)(pIe))[1] + 2)
/*
 * Return the Length of the Element data given the total IE length
 */
#define IEEE80211_ELEM_LEN(ieLen)              ((ieLen)-2)
#define TSPEC_GET_DIALOG_TOKEN_FROM_AC(_ac)    ((_ac) + 1)
#define TSPEC_GET_AC_FROM_DIALOG_TOKEN(_token) ((_token)-1)

typedef enum {
    TSPEC_STATUS_CODE_ADMISSION_ACCEPTED = 0,
    TSPEC_STATUS_CODE_ADDTS_INVALID_PARAMS = 0x1,
    TSPEC_STATUS_CODE_ADDTS_REQUEST_REFUSED = 0x3,
    TSPEC_STATUS_CODE_UNSPECIFIED_QOS_RELATED_FAILURE = 0xC8,
    TSPEC_STATUS_CODE_REQUESTED_REFUSED_POLICY_CONFIGURATION = 0xC9,
    TSPEC_STATUS_CODE_INSUFFCIENT_BANDWIDTH = 0xCA,
    TSPEC_STATUS_CODE_INVALID_PARAMS = 0xCB,
    TSPEC_STATUS_CODE_DELTS_SENT = 0x30,
    TSPEC_STATUS_CODE_DELTS_RECV = 0x31,
    TSPEC_STATUS_CODE_TSPEC_SENT_TO_AP = 0x99, /* Internal status */
    TSPEC_STATUS_CODE_INVALID = 0xff           /* TSPEC request is invalid */
} TSPEC_STATUS_CODE;

#define IEEE80211_WMM_PARAM_LEN 24
#define IEEE80211_WMM_INFO_LEN  7

#define WMM_MAX_NUM_SEQ_POOL 17 /* 4-bit tid + one for group address*/

#define NTID      (WMM_MAX_NUM_SEQ_POOL - 1)
#define MCAST_TID (WMM_MAX_NUM_SEQ_POOL - 1)

#define WMM_AC_TO_TID1(_ac) \
    (((_ac) == WMM_AC_VO) ? 6 : ((_ac) == WMM_AC_VI) ? 5 : ((_ac) == WMM_AC_BK) ? 1 : ((_ac) == WMM_AC_PSPOLL) ? 7 : 0)

#define WMM_AC_TO_TID2(_ac) (((_ac) == WMM_AC_VO) ? 7 : ((_ac) == WMM_AC_VI) ? 4 : ((_ac) == WMM_AC_BK) ? 2 : 3)

extern const uint8_t up_to_ac[];
#define TID_TO_WMM_AC(_tid) (up_to_ac[(_tid)])

#define AGGRESSIVE_MODE_SWITCH_HYSTERESIS 3  /* pkts / 100ms */
#define HIGH_PRI_SWITCH_THRESH            10 /* pkts / 100ms */
/*
 * Control frames.
 */
struct ieee80211_frame_min {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_addr1[IEEE80211_ADDR_LEN];
    uint8_t i_addr2[IEEE80211_ADDR_LEN];
    /* FCS */
} __ATTRIB_PACK;

struct ieee80211_frame_rts {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_ra[IEEE80211_ADDR_LEN];
    uint8_t i_ta[IEEE80211_ADDR_LEN];
    /* FCS */
} __ATTRIB_PACK;

struct ieee80211_frame_cts {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_ra[IEEE80211_ADDR_LEN];
    /* FCS */
} __ATTRIB_PACK;

struct ieee80211_frame_ack {
    uint8_t i_fc[2];
    uint8_t i_dur[2];
    uint8_t i_ra[IEEE80211_ADDR_LEN];
    /* FCS */
} __ATTRIB_PACK;

struct ieee80211_frame_cfend { /* NB: also CF-End+CF-Ack */
    uint8_t i_fc[2];
    uint8_t i_dur[2]; /* should be zero */
    uint8_t i_ra[IEEE80211_ADDR_LEN];
    uint8_t i_bssid[IEEE80211_ADDR_LEN];
    /* FCS */
} __ATTRIB_PACK;

/*
 * BEACON management packets
 *
 *  octet timestamp[8]
 *  octet beacon interval[2]
 *  octet capability information[2]
 *  information element
 *      octet elemid
 *      octet length
 *      octet information[length]
 */

typedef uint8_t *ieee80211_mgt_beacon_t;

#define IEEE80211_BEACON_INTERVAL(beacon)   ((beacon)[8] | ((beacon)[9] << 8))
#define IEEE80211_BEACON_CAPABILITY(beacon) ((beacon)[10] | ((beacon)[11] << 8))

#define IEEE80211_CAPINFO_ESS            0x0001
#define IEEE80211_CAPINFO_IBSS           0x0002
#define IEEE80211_CAPINFO_CF_POLLABLE    0x0004
#define IEEE80211_CAPINFO_CF_POLLREQ     0x0008
#define IEEE80211_CAPINFO_PRIVACY        0x0010
#define IEEE80211_CAPINFO_SHORT_PREAMBLE 0x0020
#define IEEE80211_CAPINFO_PBCC           0x0040
#define IEEE80211_CAPINFO_CHNL_AGILITY   0x0080
#define IEEE80211_CAPINFO_SPECTRUM_MGMT  0x0100
#define IEEE80211_CAPINFO_QOS            0x0200
#define IEEE80211_CAPINFO_SHORT_SLOTTIME 0x0400
#define IEEE80211_CAPINFO_APSD           0x0800
#define IEEE80211_CAPINFO_DSSSOFDM       0x2000
/* bits 14-15 are reserved */

#define SONY_OUI         0x460008 /* Sony OUI */
#define SONY_OUI_TYPE    0x01
#define SONY_OUI_SUBTYPE 0x01
#define SONY_OUI_VERSION 0x00

#define ATH_OUI            0x7f0300 /* Atheros OUI */
#define ATH_OUI_TYPE       0x01
#define ATH_OUI_SUBTYPE    0x01
#define ATH_OUI_VERSION_H1 0x01
#define ATH_OUI_VERSION_H2 0x02

#define CISCO_OUI               0x964000
#define CISCO_VER_OUITYPE       0x3
#define CISCO_TSM_OUITYPE       0x07
#define CISCO_RADIOMGMT_OUITYPE 0x1

#define SAMSUNG_OUI               0xfb1200 /* Samsung OUI */
#define SAMSUNG_WAC_OUITYPE       0x1
#define SAMSUNG_DEV_AD_ATTR_ID    0x0 /* device advertise attribute */
#define SAMSUNG_AUTO_PROV_ATTR_ID 0x1 /* auto provisioning attribute */
#define SAMSUNG_STATUS_CODE_ID    0x2 /* status code */
#define SAMSUNG_DEV_INFO_ATTR_ID  0x3 /* device info attribute */

#define WPA_OUI      0xf25000
#define WPA_OUI_TYPE 0x01
#define WPA_VERSION  1 /* current supported version */

#define WPA_CSE_NULL   0x00
#define WPA_CSE_WEP40  0x01
#define WPA_CSE_TKIP   0x02
#define WPA_CSE_CCMP   0x04
#define WPA_CSE_WEP104 0x05

#define WPA_ASE_NONE              0x00
#define WPA_ASE_8021X_UNSPEC      0x01
#define WPA_ASE_8021X_PSK         0x02
#define WPA_ASE_8021X_UNSPEC_CCKM 0x04
#define WPA_ASE_8021X_PSK_SHA256  0x08
#define WPA_ASE_8021X_UNSPEC_SHA256 0x10
#define WPA_ASE_8021X_SAE         0x20

#define RSN_OUI     0xac0f00
#define RSN_VERSION 1 /* current supported version */

#define RSN_CSE_NULL   0x00
#define RSN_CSE_WEP40  0x01
#define RSN_CSE_TKIP   0x02
#define RSN_CSE_WRAP   0x03
#define RSN_CSE_CCMP   0x04
#define RSN_CSE_WEP104 0x05
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
#define RSN_CSE_BIP 0x06
#endif

#define RSN_ASE_NONE             0x00
#define RSN_ASE_8021X_UNSPEC     0x01
#define RSN_ASE_8021X_PSK        0x02
#define RSN_ASE_8021X_SHA256     0x05
#define RSN_ASE_8021X_PSK_SHA256 0x06
#define RSN_ASE_SAE              0x08
#define RSN_ASE_8021X_SUITE_B_192  0x0c

#define RSN_CAP_PREAUTH 0x01
#define RSN_CAP_MFPR    0x40
#define RSN_CAP_MFPC    0x80

#define QC_OUI      0xf0fd8c /*QCN IE*/
#define QC_OUI_TYPE 0x01

#define WPS_OUI      0xF25000 /* WPS IE */
#define WPS_OUI_TYPE 0x04

#define WFA_OUI        0x9a6f50
#define OUI_WFA        0x506f9a
#define WFD_OUI_TYPE   0x09 /* WiFi-Direct IE */
#define WFDSP_OUI_TYPE 0x0a /* WiFi-Display IE */

#define TRANSITION_DISABLE_WPA3_PERSONAL 0x01

#define WMM_OUI               0xf25000
#define WMM_OUI_TYPE          0x02
#define WMM_INFO_OUI_SUBTYPE  0x00
#define WMM_PARAM_OUI_SUBTYPE 0x01
#define WMM_TSPEC_OUI_SUBTYPE 0x02
#define WMM_VERSION           1

/* WMM traffic classes */
#define WMM_AC_BK 1 /* background */
#define WMM_AC_BE 0 /* best effort */
#define WMM_AC_VI 2 /* video */
#define WMM_AC_VO 3 /* voice */
#define WMM_AC_PSPOLL                      \
    4 /* This class is used internally for \
         send pspoll/null over 5 th WHAL Q  */

/* RMF realted Params */
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
#define IEEE80211_ASSOC_COMEBACK_TYPE 3 /* Assoc Comeback timeout Interval Type */
#define IEEE80211_MMIE_KEY_ID_TYPE    4 /* IGTK MMIE KeyID type */
#define IEEE80211_IPN_LEN             6 /* IGTK IPN Len */
#define IEEE80211_MMIE_MIC_LEN        8 /* IGTK MMIE MIC Len */
#endif

#define PRIORITY_OF_AC(_ac) (((_ac) != WMM_AC_BE) ? (((_ac) != WMM_AC_BK) ? (_ac) : 0) : 1)

#define WMM_PRIORITY_COMP(_ac1, _ac2) \
    (((_ac1) != (_ac2)) ? ((PRIORITY_OF_AC(_ac1) > PRIORITY_OF_AC(_ac2)) ? 1 : -1) : 0)

/*
 * AUTH management packets
 *
 *  octet algo[2]
 *  octet seq[2]
 *  octet status[2]
 *  octet chal.id
 *  octet chal.length
 *  octet chal.text[253]
 */

#define IEEE80211_AUTH_ALGORITHM(auth)   ((auth)[0] | ((auth)[1] << 8))
#define IEEE80211_AUTH_TRANSACTION(auth) ((auth)[2] | ((auth)[3] << 8))
#define IEEE80211_AUTH_STATUS(auth)      ((auth)[4] | ((auth)[5] << 8))

#define IEEE80211_AUTH_ALG_OPEN     0x0000
#define IEEE80211_AUTH_ALG_SHARED   0x0001
#define IEEE80211_AUTH_ALG_LEAP     0x0080
#define IEEE80211_AUTH_ALG_AUTO     0x0002 /* Internal representation */
#define IEEE80211_AUTH_ALG_SAE      0x0003
#define IEEE80211_AUTH_ALG_SAE_OPEN 0x0004

enum {
    IEEE80211_AUTH_OPEN_REQUEST = 1,
    IEEE80211_AUTH_OPEN_RESPONSE = 2,
};

enum {
    IEEE80211_AUTH_SHARED_REQUEST = 1,
    IEEE80211_AUTH_SHARED_CHALLENGE = 2,
    IEEE80211_AUTH_SHARED_RESPONSE = 3,
    IEEE80211_AUTH_SHARED_PASS = 4,
};

enum {
    IEEE80211_AUTH_SAE_COMMIT = 1,   // sequence number for commit messages is "1"
    IEEE80211_AUTH_SAE_CONFIRM = 2,  // sequence number for confirm messages is "2"
};
/*
 * Reason codes
 *
 * Unlisted codes are reserved
 */

enum {
    IEEE80211_REASON_UNSPECIFIED = 1,
    IEEE80211_REASON_AUTH_EXPIRE = 2,
    IEEE80211_REASON_AUTH_LEAVE = 3,
    IEEE80211_REASON_ASSOC_EXPIRE = 4,
    IEEE80211_REASON_ASSOC_TOOMANY = 5,
    IEEE80211_REASON_NOT_AUTHED = 6,
    IEEE80211_REASON_NOT_ASSOCED = 7,
    IEEE80211_REASON_ASSOC_LEAVE = 8,
    IEEE80211_REASON_ASSOC_NOT_AUTHED = 9,

    IEEE80211_REASON_RSN_REQUIRED = 11,
    IEEE80211_REASON_RSN_INCONSISTENT = 12,
    IEEE80211_REASON_IE_INVALID = 13,
    IEEE80211_REASON_MIC_FAILURE = 14,
    IEEE80211_REASON_8021X_AUTH_FAILURE = 23,
    IEEE80211_REASON_QSTA_SETUP_REQ = 38,
    IEEE80211_REASON_QSTA_TIMEOUT = 39,

    IEEE80211_STATUS_SUCCESS = 0,
    IEEE80211_STATUS_UNSPECIFIED = 1,
    IEEE80211_STATUS_CAPINFO = 10,
    IEEE80211_STATUS_NOT_ASSOCED = 11,
    IEEE80211_STATUS_OTHER = 12,
    IEEE80211_STATUS_ALG = 13,
    IEEE80211_STATUS_SEQUENCE = 14,
    IEEE80211_STATUS_CHALLENGE = 15,
    IEEE80211_STATUS_TIMEOUT = 16,
    IEEE80211_STATUS_TOOMANY = 17,
    IEEE80211_STATUS_BASIC_RATE = 18,
    IEEE80211_STATUS_SP_REQUIRED = 19,
    IEEE80211_STATUS_PBCC_REQUIRED = 20,
    IEEE80211_STATUS_CA_REQUIRED = 21,
    IEEE80211_STATUS_SPECT_MGMT = 22,
    IEEE80211_STATUS_POWER_CAB = 23,
    IEEE80211_STATUS_SHORTSLOT_REQUIRED = 25,
    IEEE80211_STATUS_DSSSOFDM_REQUIRED = 26,
    IEEE80211_STATUS_NO_HT_SUPPORT = 27,
    IEEE80211_STATUS_ASSOC_REJECT_TEMP = 30,
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
    IEEE80211_STATUS_RMF_POLICY_VIOLATION = 31,
#endif
    IEEE80211_STATUS_REFUSED = 37,
    IEEE80211_STATUS_PARAM_INVALID = 38,
    IEEE80211_STATUS_IE_INVALID = 40,
    IEEE80211_STATUS_GRP_CIPHER = 41,
    IEEE80211_STATUS_PAIR_CIPHER = 42,
    IEEE80211_STATUS_AKMP = 43,
    IEEE80211_STATUS_INVALID_PMKID = 53,

    IEEE80211_STATUS_ANTI_CLOGGING_TOKEN_REQ = 76,
    IEEE80211_STATUS_UNSUPPORTED_FFC_GRP = 77,
    IEEE80211_STATUS_UNKNOWN_PASSWORD_IDENTIFIER = 123,
#if (defined NT_FN_WPA3)
    IEEE80211_STATUS_SAE_HASH_TO_ELEMENT = 126,
#endif
    /*
     *  Below UNDEF code will never be used in
     *  any real 802.11 frames, only reported to
     *  host in disconnect event
     */
    IEEE80211_REASON_STATUS_UNDEF = 0,
};

#define IEEE80211_WEP_KEYLEN 5 /* 40bit */
#define IEEE80211_WEP_IVLEN  3 /* 24bit */
#define IEEE80211_WEP_KIDLEN 1 /* 1 octet */
#define IEEE80211_WEP_CRCLEN 4 /* CRC-32 */
#define IEEE80211_WEP_NKID   4 /* number of key ids */

/*
 * 802.11i defines an extended IV for use with non-WEP ciphers.
 * When the EXTIV bit is set in the key id byte an additional
 * 4 bytes immediately follow the IV for TKIP.  For CCMP the
 * EXTIV bit is likewise set but the 8 bytes represent the
 * CCMP header rather than IV+extended-IV.
 */
#define IEEE80211_WEP_EXTIV    0x20
#define IEEE80211_WEP_EXTIVLEN 4 /* extended IV length */
#define IEEE80211_WEP_MICLEN   8 /* trailing MIC */

#define IEEE80211_CRC_LEN 4

/*
 * Maximum acceptable MTU is:
 *  IEEE80211_MAX_LEN - WEP overhead - CRC -
 *      QoS overhead - RSN/WPA overhead
 * Min is arbitrarily chosen > IEEE80211_MIN_LEN.  The default
 * mtu is Ethernet-compatible; it's set by ether_ifattach.
 */
#define IEEE80211_MTU_MAX 2290
#define IEEE80211_MTU_MIN 32

#define IEEE80211_MAX_LEN \
    (2300 + IEEE80211_CRC_LEN + (IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN + IEEE80211_WEP_CRCLEN))
#define IEEE80211_ACK_LEN (sizeof(struct ieee80211_frame_ack) + IEEE80211_CRC_LEN)
#define IEEE80211_MIN_LEN (sizeof(struct ieee80211_frame_min) + IEEE80211_CRC_LEN)

#define IEEE80211_AID(b)          ((b) & ~0xc000)
#define IEEE80211_AID_SET(b, w)   ((w)[IEEE80211_AID(b) / 32] |= (1 << (IEEE80211_AID(b) % 32)))
#define IEEE80211_AID_CLR(b, w)   ((w)[IEEE80211_AID(b) / 32] &= ~(1 << (IEEE80211_AID(b) % 32)))
#define IEEE80211_AID_ISSET(b, w) ((w)[IEEE80211_AID(b) / 32] & (1 << (IEEE80211_AID(b) % 32)))

/*
 * RTS frame length parameters.  The default is specified in
 * the 802.11 spec.  The max may be wrong for jumbo frames.
 */
#define IEEE80211_RTS_DEFAULT 512
#define IEEE80211_RTS_MIN     1
#define IEEE80211_RTS_MAX     IEEE80211_MAX_LEN

extern const uint8_t bcast_mac[];
extern const uint8_t zero_mac[];

#define IEEE80211_IS_MFP_FRAME(subtype)                                                              \
    (((subtype) == IEEE80211_FC0_SUBTYPE_DEAUTH) || ((subtype) == IEEE80211_FC0_SUBTYPE_DISASSOC) || \
     ((subtype) == IEEE80211_FCO_SUBTYPE_ACTION))
#if defined(FEATURE_STA_ECSA) || defined(SUPPORT_SAP_POWERSAVE)
#define QCN_IE_SAP_PS_LEN 14
#define QCN_IE_ECSA_LEN   7
#endif
#endif /* _IEEE80211_H_ */
