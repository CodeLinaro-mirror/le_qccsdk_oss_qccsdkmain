
#ifndef WLAN_QAPI_HELPER_H
#define WLAN_QAPI_HELPER_H

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
extern void wlan_clear_privacy(void);
extern void wlan_set_connect_ssid (const unsigned char *ssid, uint8_t ssidLength);
extern void wlan_set_connect_bssid (const uint8_t *bssid, uint8_t bssid_length);
extern void wlan_set_passphrase (const uint8_t *passphrase, uint8_t passphrase_len);
extern void wlan_set_scan_param (WMI_START_SCAN_CMD *p_cmd, const qapi_WLAN_Start_Scan_Params_t *scan_Params);
extern void wlan_preset_specific_param (void);
extern int32_t wlan_set_channel(uint8_t device_id, uint16_t channel);
extern int32_t wlan_set_country_code(uint8_t device_id, uint8_t *country_code);
extern int32_t wlan_set_phy_mode(uint8_t device_id, uint32_t phy_mode);
extern int32_t wlan_set_11n_ht(uint8_t  __attribute__((__unused__)) device_id, uint8_t htconfig);
extern int32_t wlan_set_op_mode(uint8_t mode);
extern qapi_Status_t wlan_get_mac_address(uint8_t __attribute__((__unused__)) device_ID, uint8_t mac_addr[__QAPI_WLAN_MAC_LEN]);
extern qapi_Status_t wlan_get_power_mode(uint8_t __attribute__((__unused__)) device_ID, uint8_t *powermode);
extern qapi_Status_t wlan_get_phy_mode(uint8_t *phymode);
extern qapi_Status_t wlan_sta_get_rssi(uint8_t device_ID, uint8_t *rssi);
extern qapi_Status_t wlan_sta_get_reg_info(qapi_WLAN_Reg_Evt_t *regulatory);
#endif //WLAN_QAPI_HELPER_H

