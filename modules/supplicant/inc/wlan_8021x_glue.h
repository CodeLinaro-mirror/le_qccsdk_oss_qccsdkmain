/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_8021X_GLUE_H_
#define _WLAN_8021X_GLUE_H_

int wlan_8021x_eap_register_methods(void);
#define wlan_8021x_eap_unregister_methods eap_peer_unregister_methods

int wlan_8021x_init_eapol(wlan_8021x_intf_t *wl8021x_intf);
void wlan_8021x_deinit_eapol(wlan_8021x_intf_t *wl8021x_intf);
void wlan_8021x_initiate_eapol(wlan_8021x_intf_t *wl8021x_intf);
void wlan_8021x_rx_eapol(wlan_8021x_intf_t *wl8021x_intf,
                         const unsigned char *src_addr,
                         const unsigned char *buf, unsigned int len);

void wlan_8021x_rx_eapol_key_notify(wlan_8021x_intf_t *wl8021x_intf);
int wlan_8021x_force_eapol_success(wlan_8021x_intf_t *wl8021x_intf, int type);

#endif /* _WLAN_8021X_GLUE_H_ */
