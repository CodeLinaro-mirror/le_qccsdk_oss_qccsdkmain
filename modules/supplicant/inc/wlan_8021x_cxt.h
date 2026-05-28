/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_8021X_CXT_H_
#define _WLAN_8021X_CXT_H_

#include "includes.h"
#include "common.h"
#include "eap_peer/eap_config.h"

struct eapol_sm;
struct eap_peer_config;

typedef struct {
  int eapol_packet_buffer_size;
  unsigned int eap_workaround;
  char eapol_version;
  char fast_reauth;
  unsigned int input_method_cnt;
} wlan_8021x_global_config_t;

typedef struct {
  struct eap_peer_config eap;
} wlan_8021x_intf_config_t;

struct wlan_8021x_global_s;

typedef enum {
  WL8021X_INIT = 10,
  WL8021X_AUTHENTICATING,
  WL8021X_AUTHENTICATED,
  WL8021X_PMK_CACHED,
  WL8021X_AUTHENTICAT_FAILED,
} WL8021X_STATE_e;

typedef enum {
  EAPOL_FORCE_TYPE_PMKID = 1, /* Use cached pmkid */
} EAPOL_FORCE_SUCCESS_TYPE_e;

typedef struct wlan_8021x_intf_s {
  struct wlan_8021x_global_s *wl8021x_global;
  wlan_8021x_intf_config_t wl8021x_intf_cfg;
  void *suppl_intf;
  unsigned char wl8021x_intf_id;
  int eapol_sock;
  int eapol_received;
  WL8021X_STATE_e wl8021x_intf_state;
  struct eapol_sm *eapol;
} wlan_8021x_intf_t;

typedef struct wlan_8021x_global_s {
  void *wlan_qapi_cxt;
  void *suppl_global;
  wlan_8021x_global_config_t wl8021x_global_cfg;
  wlan_8021x_intf_t *wl8021x_intfs;
  int intfs_cnt;
  unsigned short eapol_protocol;
} wlan_8021x_global_t;

struct ethhdr {
  uint8_t h_dest[ETH_ALEN];
  /**< Destination address. */

  uint8_t h_source[ETH_ALEN];
  /**< Source address. */

  uint16_t h_proto;
  /**< Protocol id. */

} __attribute__((packed));

/**
 * @brief Device-independent link-layer address.
 */
struct sockaddr_ll {
  uint16_t sll_family;
  /**< Always AF_PACKET. */

  uint16_t sll_protocol;
  /**< Link-layer protocol id in network byte order, e.g. htons(0x888e) for
   * EAPOL. */

  int sll_ifindex;
  /**< ifIndex in rfc1213-mib2 (1-based). */

  uint16_t sll_hatype;
  /**< ARP hardware type. */

  uint8_t sll_pkttype;
  /**< Packet type. */

  uint8_t sll_halen;
  /**< Size (in bytes) of link-layer address. */

  uint8_t sll_addr[8];
  /**< Link-layer address. */
};

#define WL8021X_2_WLANCXT(global)                                              \
  (((wlan_8021x_global_t *)(global))->wlan_qapi_cxt)
#define WL8021X_2_SUPPL(global)                                                \
  (((wlan_8021x_global_t *)(global))->suppl_global)
#define WL8021X_INTF_2_SUPPL_INTF(intf)                                        \
  (((wlan_8021x_intf_t *)(intf))->suppl_intf)

#define WL8021X_INTF_STATE(wl8021x_intf)                                       \
  (((wlan_8021x_intf_t *)wl8021x_intf)->wl8021x_intf_state)

#endif /* _WLAN_8021X_CXT_H_ */
