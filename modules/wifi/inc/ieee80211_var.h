/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _IEEE80211_VAR_H_
#define _IEEE80211_VAR_H_

/*
 * Definitions for IEEE 802.11 drivers.
 */

#include "ieee80211.h"
#include "osapi.h"

#define CONFIG_NO_INLINE

/* inline - define as  or just define it to be empty, if needed */
#ifdef CONFIG_NO_INLINE
#define inline
#else
#define inline
#endif

/*
 * Transmitted frames have the following information
 * held in the tx/rx buf control buffer.  This is used to
 * communicate various inter-procedural state that needs
 * to be associated with the frame for the duration of
 * it's existence.
 *
 */

#ifdef CONFIG_WIFILIB_6GHZ
typedef struct ieee80211_he_op_ie {
    uint ie_he_op_param : 24;
    uint8_t ie_bss_color;
    uint16_t ie_he_mcs;
    uint8_t ie_6g_channel;
    uint32_t ie_6g_info;
} __attribute__((packed)) ieee80211_he_op_ie_t;
#endif

struct ieee80211_common_ie {
    uint16_t ie_chan;
    uint8_t *ie_tstamp;
    uint8_t *ie_ssid;
    uint8_t *ie_rates;
    uint8_t *ie_xrates;
    uint8_t *ie_country;
    uint8_t *ie_wpa;
    uint8_t *ie_rsn;
    uint8_t *ie_rsnx;
#ifdef NT_FN_WMM
    uint8_t *ie_wmminfo;
    uint8_t *ie_wmmparam;
#endif  // NT_FN_WMM
    uint8_t *ie_uapsd;
    uint8_t *ie_ath;
    uint16_t ie_capInfo;
    uint16_t ie_beaconInt;
    uint8_t *ie_tim;
    uint8_t *ie_chswitch;
    uint8_t *ie_extchswitch;
    uint8_t *ie_txpower;
    uint8_t *ie_qbss;
    uint8_t *ie_erp;
    uint16_t ie_ibssParam;
    uint8_t ie_c_c_x_unused_version; /* == 0 if AP is not cisco */
    uint8_t *ie_wapi;
    uint8_t *ie_apchanrep;
    uint8_t *ie_wps;
    uint8_t *ie_radiomgmt_cap;
    uint8_t *ie_ht_cap_cmn;
    uint8_t *ie_ht_info_cmn;
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
    uint8_t *ie_wur_cap;
    uint8_t *wur_capabilities_element; /* wur capability element */
    uint8_t *wur_operation_element;    /* wur capability element */
#endif

#ifdef NT_FN_WNM_POWERSAVE_MODE
    uint8_t *ie_wnm_cap; /* wnm capability element */
#endif                   /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
    uint8_t *ie_twt_cap; /* twt capability element */
#endif                   /* NT_FN_TWT */
    uint8_t *ie_ext_cap;
    uint8_t *ie_he_cap;
    struct chanAccParams chanParams;
#ifdef CONFIG_WIFILIB_6GHZ
    ieee80211_he_op_ie_t *ie_he_op;
#endif
};

#define IEEE80211_TXPOWER_MAX 100 /* .5 dbM (XXX units?) */
#define IEEE80211_TXPOWER_MIN 0   /* kill radio */

#define IEEE80211_DTIM_MAX     15 /* max DTIM period */
#define IEEE80211_DTIM_MIN     1  /* min DTIM period */
#define IEEE80211_DTIM_DEFAULT 1  /* default DTIM period */

#define IEEE80211_BINTVAL_MAX     500 /* max beacon interval (ms) */
#define IEEE80211_BINTVAL_MIN     25  /* min beacon interval */
#define IEEE80211_BINTVAL_DEFAULT 100 /* default beacon interval */

#define IEEE80211_PS_SLEEP     0x1 /* STA is in power saving mode */
#define IEEE80211_PS_MAX_QUEUE 50  /* maximum saved packets */

#define IEEE80211_MS_TO_TU(x) (((x)*1000) / 1024)
#define IEEE80211_TU_TO_MS(x) (((x)*1024) / 1000)

#define IEEE80211_RATE2MBS(r) (((r)&IEEE80211_RATE_VAL) / 2)

#define IEEE80211_ADDR_EQ(a1, a2)     (memcmp(a1, a2, IEEE80211_ADDR_LEN) == 0)
#define IEEE80211_ADDR_COPY(dst, src) memscpy(dst, IEEE80211_ADDR_LEN, src, IEEE80211_ADDR_LEN)
#define IEEE80211_ADDR_NULL(addr)     ((!addr[0]) && (!addr[1]) && (!addr[2]) && (!addr[3]) && (!addr[4]) && (!addr[5]))
#define IEEE80211_ADDR_BROADCAST(addr)                                                                        \
    ((addr[0] == 0xFF) && (addr[1] == 0xFF) && (addr[2] == 0xFF) && (addr[3] == 0xFF) && (addr[4] == 0xFF) && \
     (addr[5] == 0xFF))
/* ic_flags */
/* NB: bits 0x6f available */
/* NB: this is intentionally setup to be IEEE80211_CAPINFO_PRIVACY */
#define IEEE80211_F_PRIVACY 0x00000010 /* CONF: privacy enabled */
#define IEEE80211_F_SCAN    0x00000080 /* STATUS: scanning */
#define IEEE80211_F_ASCAN   0x00000100 /* STATUS: active scan */
#define IEEE80211_F_SIBSS   0x00000200 /* STATUS: start IBSS */
/* NB: this is intentionally setup to be IEEE80211_CAPINFO_SHORT_SLOTTIME */
#define IEEE80211_F_SHSLOT      0x00000400 /* STATUS: use short slot time*/
#define IEEE80211_F_PMGTON      0x00000800 /* CONF: Power mgmt enable */
#define IEEE80211_F_DESBSSID    0x00001000 /* CONF: des_bssid is set */
#define IEEE80211_F_WMM         0x00002000 /* CONF: enable WMM use */
#define IEEE80211_F_ROAMING     0x00004000 /* CONF: roaming enabled */
#define IEEE80211_F_SWRETRY     0x00008000 /* CONF: sw tx retry enabled */
#define IEEE80211_F_TXPOW_FIXED 0x00010000 /* TX Power: fixed rate */
#define IEEE80211_F_OTP_MACADDR 0x00020000 /* CONF: MAC Addr initialized from OTP */
#define IEEE80211_F_SHPREAMBLE  0x00040000 /* STATUS: use short preamble */
#define IEEE80211_F_DATAPAD     0x00080000 /* CONF: do alignment pad */
#define IEEE80211_F_USEERPROT   0x00100000 /* STATUS: ERP protection enabled */
#define IEEE80211_F_USEHTPROT   0x00200000 /* STATUS: HT protection enabled*/
#define IEEE80211_F_TIMUPDATE   0x00400000 /* STATUS: update beacon tim */
#define IEEE80211_F_WPA1        0x00800000 /* CONF: WPA enabled */
#define IEEE80211_F_WPA2        0x01000000 /* CONF: WPA2 enabled */
#define IEEE80211_F_WPA         0x01800000 /* CONF: WPA/WPA2 enabled */
#define IEEE80211_F_DROPUNENC   0x02000000 /* CONF: drop unencrypted */
#define IEEE80211_F_COUNTERM    0x04000000 /* CONF: TKIP countermeasures */
#define IEEE80211_F_HIDESSID    0x08000000 /* CONF: hide SSID in beacon */
#define IEEE80211_F_NOBRIDGE    0x10000000 /* CONF: dis. internal bridge */
#define IEEE80211_F_WMMUPDATE   0x20000000 /* STATUS: update beacon wmm */
#define IEEE80211_F_CHANSWITCH  0x40000000 /* STATUS: Add beacon CSA */

/* ic_caps */
#define IEEE80211_C_WEP        0x00000001 /* CAPABILITY: WEP available */
#define IEEE80211_C_TKIP       0x00000002 /* CAPABILITY: TKIP available */
#define IEEE80211_C_AES        0x00000004 /* CAPABILITY: AES OCB avail */
#define IEEE80211_C_AES_CCM    0x00000008 /* CAPABILITY: AES CCM avail */
#define IEEE80211_C_CKIP       0x00000020 /* CAPABILITY: CKIP available */
#define IEEE80211_C_PMGT       0x00000200 /* CAPABILITY: Power mgmt */
#define IEEE80211_C_SWRETRY    0x00001000 /* CAPABILITY: sw tx retry */
#define IEEE80211_C_TXPMGT     0x00002000 /* CAPABILITY: tx power mgmt */
#define IEEE80211_C_SHSLOT     0x00004000 /* CAPABILITY: short slottime */
#define IEEE80211_C_SHPREAMBLE 0x00008000 /* CAPABILITY: short preamble */
#define IEEE80211_C_TKIPMIC    0x00020000 /* CAPABILITY: TKIP MIC avail */
#define IEEE80211_C_WPA1       0x00800000 /* CAPABILITY: WPA1 avail */
#define IEEE80211_C_WPA2       0x01000000 /* CAPABILITY: WPA2 avail */
#define IEEE80211_C_WPA        0x01800000 /* CAPABILITY: WPA1+WPA2 avail*/
#define IEEE80211_C_BURST      0x02000000 /* CAPABILITY: frame bursting */
#define IEEE80211_C_WMM        0x04000000 /* CAPABILITY: WMM avail */
/* XXX protection/barker? */

#define IEEE80211_C_CRYPTO 0x0000002f /* CAPABILITY: crypto alg's */

/* Verify the existence and length of __elem or get out. */
#ifdef NT_FN_PRODUCTION_STATS
#define IEEE80211_VERIFY_ELEMENT(dev, __elem, __maxlen, status) \
    do {                                                        \
        if ((__elem) == NULL) {                                 \
            DEV_IEEE_STATS_INC((dev), is_rx_elem_missing);      \
            return (status);                                    \
        }                                                       \
        if ((__elem)[1] > (__maxlen)) {                         \
            DEV_IEEE_STATS_INC((dev), is_rx_elem_toobig);       \
            return (status);                                    \
        }                                                       \
    } while (0)
#define IEEE80211_VERIFY_LENGTH(dev, _len, _minlen, status) \
    do {                                                    \
        if ((_len) < (_minlen)) {                           \
            DEV_IEEE_STATS_INC((dev), is_rx_elem_toosmall); \
            return (status);                                \
        }                                                   \
    } while (0)
#define IEEE80211_VERIFY_SSID(dev, _sp, _ssid, status)                                                                 \
    do {                                                                                                               \
        if ((_ssid)[1] != 0 && ((_ssid)[1] != (_sp)->ssid_len || memcmp((_ssid) + 2, (_sp)->ssid, (_ssid)[1]) != 0)) { \
            DEV_IEEE_STATS_INC((dev), is_rx_ssidmismatch);                                                             \
            return (status);                                                                                           \
        }                                                                                                              \
    } while (0)
#else
#define IEEE80211_VERIFY_ELEMENT(dev, __elem, __maxlen, status) \
    do {                                                        \
        if ((__elem) == NULL) {                                 \
            return (status);                                    \
        }                                                       \
        if ((__elem)[1] > (__maxlen)) {                         \
            return (status);                                    \
        }                                                       \
    } while (0)
#define IEEE80211_VERIFY_LENGTH(dev, _len, _minlen, status) \
    do {                                                    \
        if ((_len) < (_minlen)) {                           \
            return (status);                                \
        }                                                   \
    } while (0)
#define IEEE80211_VERIFY_SSID(dev, _sp, _ssid, status)                                                                 \
    do {                                                                                                               \
        if ((_ssid)[1] != 0 && ((_ssid)[1] != (_sp)->ssid_len || memcmp((_ssid) + 2, (_sp)->ssid, (_ssid)[1]) != 0)) { \
            return (status);                                                                                           \
        }                                                                                                              \
    } while (0)
#endif  // NT_FN_PRODUCTION_STATS
static __inline int isXR(const uint8_t *frm)
{
    uint8_t nrates = frm[1];
    uint8_t i;

    for (i = 0; i < nrates; i++) {
        if (frm[i + 2] == 0x81) {
            return 1;
        }
    }

    return 0;
}

static __inline int iswpaoui(const uint8_t *frm)
{
    return frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((WPA_OUI_TYPE << 24) | WPA_OUI);
}

static __inline int iswmmoui(const uint8_t *frm)
{
    return frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((WMM_OUI_TYPE << 24) | WMM_OUI);
}

static __inline int iswmmparam(const uint8_t *frm)
{
    return frm[1] > 5 && frm[6] == WMM_PARAM_OUI_SUBTYPE;
}

static __inline int iswmminfo(const uint8_t *frm)
{
    return frm[1] > 5 && frm[6] == WMM_INFO_OUI_SUBTYPE;
}

static __inline int isatherosoui(const uint8_t *frm)
{
    return (frm[1] > 3 && (NT_LE_READ_4(frm + 2) == ((ATH_OUI_TYPE << 24) | ATH_OUI)) && (frm[6] == ATH_OUI_SUBTYPE) &&
            ((frm[7] == ATH_OUI_VERSION_H1) || (frm[7] == ATH_OUI_VERSION_H2)));
}
static __inline int is_ciscover_oui(const uint8_t *frm)
{
    return (frm[1] > 4 && (NT_LE_READ_4(frm + 2) == ((CISCO_VER_OUITYPE << 24) | CISCO_OUI)));
}

static __inline int is_p2p_oui(const uint8_t *frm)
{
    return ((frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((WFD_OUI_TYPE << 24) | WFA_OUI)) ||
            (frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((WFDSP_OUI_TYPE << 24) | WFA_OUI)));
}

static __inline int iswpsoui(const uint8_t *frm)
{
    return frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((WPS_OUI_TYPE << 24) | WPS_OUI);
}

static __inline int is_qcn_oui(const uint8_t *frm)
{
    return frm[1] > 3 && NT_LE_READ_4(frm + 2) == ((QC_OUI_TYPE << 24) | QC_OUI);
}

static __inline int is_qcn_oui_wo_type(const uint8_t *frm)
{
    return (NT_LE_READ_4(frm) & 0xFFFFFF) == (QC_OUI);
}

static __inline int is_ciscorm_ie(const uint8_t *frm)
{
    return (frm[1] > 4 && (NT_LE_READ_4(frm + 2) == ((CISCO_RADIOMGMT_OUITYPE << 24) | CISCO_OUI)));
}

static __inline int32_t iswmmtspec(const uint8_t *frm)
{
    return frm[1] > 5 && frm[6] == WMM_TSPEC_OUI_SUBTYPE;
}

static __inline int iscisco_tsm_oui(const uint8_t *frm)
{
    return (frm[1] > 4 && (NT_LE_READ_4(frm + 2) == ((CISCO_TSM_OUITYPE << 24) | CISCO_OUI)));
}

static __inline int is_samsung_ie(const uint8_t *frm)
{
    extern uint8_t dev_info_len;

    if (0 == dev_info_len) {
        return (frm[1] >= 7 && (NT_LE_READ_4(frm + 2) == ((SAMSUNG_WAC_OUITYPE << 24) | SAMSUNG_OUI)));
    } else {
        return (frm[1] >= 9 && (NT_LE_READ_4(frm + 2) == ((SAMSUNG_WAC_OUITYPE << 24) | SAMSUNG_OUI)));
    }
}

#define IEEE80211_NODE_STAT(_a, _b)

#endif /* _IEEE80211_VAR_H_ */
