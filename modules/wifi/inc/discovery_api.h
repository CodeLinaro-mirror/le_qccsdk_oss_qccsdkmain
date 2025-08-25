/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _DISCOVERY_API_H_
#define _DISCOVERY_API_H_
#include "discovery.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
/* Forward declarations */
struct channel;
struct conn_profile_s;
struct ieee80211_common_ie;
struct ieee80211_frame;

#define DC_DETERMINISTIC_INTERVAL_DEFAULT    100
#define DC_DETERMINISTIC_INTERVAL_CONNECTED  80
#define DC_ACTIVE_CHAN_DWELL_TIME_DEFAULT    40  // 20
#define DC_ACTIVE_CHAN_DWELL_TIME_CONNECTED  20
#define DC_PASSIVE_CHAN_DWELL_TIME_DEFAULT   60  // 50
#define DC_PASSIVE_CHAN_DWELL_TIME_CONNECTED 30
#define DC_CHAN_DWELL_TIME_MIN               DC_ACTIVE_CHAN_DWELL_TIME_DEFAULT
#define MAX_PROBED_SSIDS                     (MAX_PROBED_SSID_INDEX + 1)

typedef enum {
    DC_SCAN_TYPE_ID = 0,
    DC_PROFILE_FILTER_ID,
    DC_CHANNEL_HINT_ID,
    DC_DETERMINISTIC_SCAN_INTERVAL_ID,
    DC_CURRENT_REG_CODE_ID,
    DC_REG_11D_ENABLE_ID,
    DC_SCAN_LIST_SIZE_ID,
} DC_CONFIG_ID;

#define DC_SET_SCAN_TYPE(dev, type)                     \
    do {                                                \
        uint32_t config = (type);                       \
        dc_set_config((dev), DC_SCAN_TYPE_ID, &config); \
    } while (0);
#define DC_GET_SCAN_TYPE(dev, type)                             \
    do {                                                        \
        dc_get_config((dev), DC_SCAN_TYPE_ID, (void *)&(type)); \
    } while (0);
#define DC_GET_SCAN_LIST_SIZE_ID(dev, type)                          \
    do {                                                             \
        dc_get_config((dev), DC_SCAN_LIST_SIZE_ID, (void *)&(type)); \
    } while (0);

#define DC_SET_PROFILE_FILTER(dev, filter)                   \
    do {                                                     \
        uint32_t config = (filter);                          \
        dc_set_config((dev), DC_PROFILE_FILTER_ID, &config); \
    } while (0);
#define DC_GET_PROFILE_FILTER(dev, filter)                             \
    do {                                                               \
        dc_get_config((dev), DC_PROFILE_FILTER_ID, (void *)&(filter)); \
    } while (0);

#define DC_SET_CHANNEL_HINT(dev, channel)                                 \
    do {                                                                  \
        uint8_t _chindex = dc_freq_to_chindex((dev->pDevCmn), (channel)); \
        dc_set_config((dev), DC_CHANNEL_HINT_ID, &(_chindex));            \
        dc_init_scan_list((dev));                                         \
    } while (0);
#define DC_GET_CHANNEL_HINT(dev, channel)                          \
    do {                                                           \
        uint8_t _chindex;                                          \
        uint32_t config;                                           \
        dc_get_config((dev), DC_CHANNEL_HINT_ID, (void *)&config); \
        _chindex = *((uint8_t *)&config);                          \
        channel = NULL;                                            \
        if (_chindex != DC_CHANNEL_INDEX_INVALID) {                \
            channel = DEV_GET_CHANNEL(dev, _chindex);              \
        }                                                          \
    } while (0);

#define DC_SET_DETERMINISTIC_SCAN_INTERVAL(dev, interval)                 \
    do {                                                                  \
        uint32_t config = (interval);                                     \
        dc_set_config((dev), DC_DETERMINISTIC_SCAN_INTERVAL_ID, &config); \
    } while (0);

#define DC_GET_CURRENT_REG_CODE(dev, regcode)                             \
    do {                                                                  \
        dc_get_config((dev), DC_CURRENT_REG_CODE_ID, (void *)&(regcode)); \
    } while (0);

#define DC_SET_REG_11D_CONFIG(dev, enable)                   \
    do {                                                     \
        uint32_t config = (enable);                          \
        dc_set_config((dev), DC_REG_11D_ENABLE_ID, &config); \
    } while (0);

void *dc_init(dev_common_t *pDevCmn, devh_t *dev);
void nt_devcfg_dc(devh_t *dev);
void dc_deinit(devh_t *dev);
void dc_deterministic_scan_timeout(TimerHandle_t thandle);
void dc_scan_opportunity(devh_t *);
#if defined(CONFIG_CHANNEL_SCHEDULER)
nt_status_t dc_begin_scan(devh_t *dev, DC_SCAN_TYPE type, uint32_t priority, CSERV_COMPLETION_CB cb, void *arg);
#else
nt_status_t dc_begin_scan(devh_t *, DC_SCAN_TYPE type, void (*cb)(void *arg, nt_status_t status), void *arg);
#endif /* CONFIG_CHANNEL_SCHEDULER */
void dc_cancel_scan_cb(void *arg, nt_status_t status);
void dc_end_scan(devh_t *, nt_status_t status);
void dc_cancel_scan(devh_t *dev);
void dc_min_chdwell_timeout(TimerHandle_t thandle);
void dc_update_scan_list(devh_t *, uint8_t chindex);
void dc_reset_scan_list(devh_t *);
void dc_init_scan_list(devh_t *);
void dc_print_scan_list(devh_t *);
void dc_scan_next_channel(devh_t *);
uint8_t dc_get_next_channel(devh_t *);
void dc_dpm_resume_rsp_handler(uint8_t __unused status);
void dc_scan_channel(devh_t *, uint8_t chindex);
void dc_scan_channel_finish(void *arg, nt_status_t status);
void dc_ssid_probe_add(devh_t *, const char *ssid, uint8_t length, uint8_t flag);
void dc_ssid_probe_list_print(devh_t *dev);
void dc_ssid_probe_remove(devh_t *, uint8_t index);
void dc_send_next_ssid_probe(devh_t *);
NT_BOOL dc_ssid_probe_list_match(devh_t *dev, unsigned char *rcvd_ssid, uint8_t rcvd_ssid_len);
void dc_set_scan_params(devh_t *, uint32_t chdwell_minact_duration, uint32_t chdwell_pas_duration);
void dc_reset_channel_priority(devh_t *, uint8_t priority);
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
NT_BOOL dc_profile_check(devh_t *, struct ieee80211_common_ie *cie, __unused NT_BOOL probe, uint8_t *wpaie_flag,
                         uint8_t *mcipher, uint8_t *ucipher, uint8_t *authmode);
#else
NT_BOOL dc_profile_check(devh_t *, struct ieee80211_common_ie *cie, __unused NT_BOOL probe, uint8_t *wpaie_flag,
                         uint8_t *mcipher, uint8_t *ucipher, uint8_t *authmode);
#endif  // NT_FN_RMF

/*@brief: dc_cipher_check()-performs cipher match on STA side, according to the capabilities of the BSS and
 * connection profile of the device(STA)
 * @param: global device structure, conn. profile, Rx Frame, RMF capability frame
 * @returns: Boolean Value (as per the result of cipher-match)
 */
NT_BOOL dc_cipher_match(devh_t *, conn_profile_t *conn, struct ieee80211_common_ie *cie, uint8_t *rsn_cap_flag,
                        uint8_t *wpaie_flag, uint8_t *mcipher, uint8_t *ucipher, uint8_t *authmode);
NT_BOOL dc_basic_rate_check(devh_t *, struct ieee80211_common_ie *cie);
NT_BOOL dc_hidden_ssid_check(devh_t *, conn_profile_t *profile, uint8_t *ssid, uint32_t length);
nt_status_t dc_beacon_receive(devh_t *, uint8_t *abf, NT_BOOL isProbe, uint8_t rssi, struct ieee80211_common_ie *cie);
void dc_update_static_bss_attributes(devh_t *, struct bss_s *bss, struct ieee80211_common_ie *cie,
                                     struct ieee80211_frame *wh);
#ifdef NT_FN_WPA3
void dc_update_dynamic_bss_attributes(devh_t *, struct bss_s *bss, struct ieee80211_common_ie *cie,
                                      NT_BOOL profileMatch, uint8_t wpaie_flag, uint8_t mcipher, uint8_t ucipher,
                                      uint8_t auth_mode);
#else
void dc_update_dynamic_bss_attributes(devh_t *, struct bss_s *bss, struct ieee80211_common_ie *cie,
                                      NT_BOOL profileMatch, uint8_t wpaie_flag, uint8_t mcipher, uint8_t ucipher,
                                      uint8_t auth_mode);
#endif  // NT_FN_WPA3

uint8_t dc_force_country_code(devh_t *, uint8_t *param);
void dc_wreg_set_regcode(devh_t *);
void dc_wreg_regnode_clear(devh_t *);
void dc_wreg_new_reg(devh_t *, NT_REG_CODE regcode, WMI_PHY_MODE phyMode);
void dc_wreg_beacon_country_info(devh_t *, struct ieee80211_country_ie *country, uint8_t rssi);
void dc_wreg_regnode_add(devh_t *, struct ieee80211_country_ie *country, uint8_t rssi);
void dc_get_suppchan_ie(devh_t *, struct ieee80211_suppchan_ie *sie);
int8_t dc_wreg_countryIe_txPwr(devh_t *, struct ieee80211_country_ie *cie, uint32_t chNum);
void dc_set_channel_list(devh_t *, WMI_PHY_MODE mode, uint16_t *clist, uint32_t clist_size);
uint32_t dc_get_channel_list(devh_t *, uint16_t *clist, uint32_t clist_size);
uint8_t dc_freq_to_chindex(dev_common_t *pDevCmn, uint32_t frequency);
struct channel *dc_freq_to_chan(devh_t *, uint32_t frequency);
void dc_clear_scan_list(devh_t *);
int dc_get_active_scan_list_cnt(devh_t *dev);
void dc_request_preauth(devh_t *);
struct channel *dc_select_channel(devh_t *, NETWORK_TYPE mode);
void dc_set_config(devh_t *, DC_CONFIG_ID id, void *config);
uint32_t dc_get_config(devh_t *, DC_CONFIG_ID id, void *config);
uint16_t dc_powercap_maxpower(devh_t *, uint16_t bss_power);
NT_BOOL dc_is_scanning_curchan(devh_t *);
void *dc_get_addr(devh_t *dev, DC_CONFIG_ID id, uint32_t *length);

nt_status_t dc_disable_channel(devh_t *dev, uint32_t frequency);

#ifdef CONFIG_CHANNEL_SCHEDULER
void dc_scan_channel_start(void *arg, nt_status_t status);
#else
nt_status_t dc_scan_channel_start(void *arg, uint8_t chindex);
#endif

#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
nt_status_t dc_update_scan_result(devh_t *dev, ap_info *bss);
uint8_t dc_get_scan_id(devh_t *dev);
#endif

#ifdef SUPPORT_5GHZ
#ifdef SUPPORT_REGULATORY
nt_status_t dc_init_freq_bands(dev_common_t *pDevCmn, uint8_t *chan_list_size);
#else
nt_status_t dc_init_freq_bands(uint8_t *chan_list_size);
#endif
uint8_t dc_get_chidx_from_freq(uint16_t freq);
#endif

#ifdef SUPPORT_EVENT_HANDLERS
typedef void (*wlan_dc_event_handler)(devh_t *dev, DC_EVENT *event);
void dc_deliver_event(devh_t *dev, uint8_t type, uint32_t dwell_time);
nt_status_t wlan_dc_register_event_handler(devh_t *dev, wlan_dc_event_handler evhandler);
nt_status_t wlan_dc_unregister_event_handler(devh_t *dev, wlan_dc_event_handler evhandler);
#endif /* SUPPORT_EVENT_HANDLERS */

void dc_set_channel_filter(devh_t *dev, WLAN_PHY_MODE mode);

#endif /* _DISCOVERY_API_H_ */
