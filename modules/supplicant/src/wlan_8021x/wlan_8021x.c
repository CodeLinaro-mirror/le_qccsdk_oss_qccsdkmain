/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_wlan_base.h"
#include "sockets.h"
#include "if_ethersubr.h"
#include "supplicant_cxt.h"
#include "wlan_8021x_cxt.h"
#include "wlan_8021x_cfg.h"
#include "wlan_8021x.h"
#include "wlan_8021x_glue.h"
#include "qapi/qapi_wlan.h"
#include "eapol_supp/eapol_supp_sm.h"
#include "eap_peer/eap.h"
#include "eap_peer/eap_methods.h"
#include "qapi_status.h"
#include "qcc730_os.h"
#include "wlan_drv.h"

/* IEEE 802.11, 8.5.2 EAPOL-Key frames */
#define WPA_KEY_INFO_KEY_TYPE BIT(3) /* 1 = Pairwise, 0 = Group key */
#define WPA_KEY_INFO_ACK BIT(7)
#define WPA_KEY_INFO_MIC BIT(8)
#define WPA_KEY_INFO_SECURE BIT(9)
#define ID_IN_RANGE(x, min, max)                                               \
  ((((uint16_t)(x)) >= ((uint16_t)(min))) &&                                   \
   (((uint16_t)(x)) <= ((uint16_t)(max))))

wlan_8021x_global_t *g_wl8021x_global = NULL;
// wlan_lib_cblist_t g_wl8021x_cblist = {NULL};
bool wlan_lib_auth_is_8021x(unsigned int auth) {
  if (auth & (WMI_WPA_AUTH | WMI_WPA2_AUTH | WMI_WPA2_SHA256_AUTH | WMI_WPA3_ENTERPRISE_ONLY_AUTH)) {
    return true;
  } else {
    return false;
  }
}

static int wlan_8021x_mode_switch_prepare_cb(unsigned int dest_mode_id,
                                             void *cb_cxt) {
  wlan_8021x_global_t *wl8021x_global = (wlan_8021x_global_t *)cb_cxt;

  info_printf("%s\n", __FUNCTION__);
  if (wl8021x_global && wl8021x_global->wl8021x_intfs) {
    int i;
    wlan_8021x_intf_t *wl8021x_intf = NULL;
    int intfs_cnt = wl8021x_global->intfs_cnt;

    for (i = 0; i < intfs_cnt; i++) {
      wl8021x_intf = &wl8021x_global->wl8021x_intfs[i];
      if (wl8021x_intf->eapol_sock) {
        info_printf("%s close socket\n", __FUNCTION__);
        closesocket(wl8021x_intf->eapol_sock);
        wl8021x_intf->eapol_sock = 0;
      }
    }
  }
  return 0;
}

static void wlan_8021x_mode_switch_cancel_cb(void *cb_cxt) {
  wlan_8021x_global_t *global = (wlan_8021x_global_t *)cb_cxt;

  info_printf("%s\n", __FUNCTION__);

  if (global && global->wl8021x_intfs) {
    int i;
    wlan_8021x_intf_t *wl8021x_intf = NULL;
    suppl_intf_t *suppl_intf = NULL;
    int intfs_cnt = global->intfs_cnt;
    struct sockaddr_ll local_addr;
    int ret = QAPI_ERROR;
    int sock = 0;

    for (i = 0; i < intfs_cnt; i++) {
      suppl_intf = (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);
      wl8021x_intf = &global->wl8021x_intfs[i];
      sock = socket(AF_PACKET, SOCK_RAW, global->eapol_protocol);
      if (!suppl_intf) continue;
      if (sock == QAPI_ERROR) {
        warn_printf("%s %d\n", __FUNCTION__, __LINE__);
        continue;
      }
      wl8021x_intf->eapol_sock = sock;

      os_memzero(&local_addr, sizeof(local_addr));
      local_addr.sll_family = AF_PACKET;
      local_addr.sll_protocol = global->eapol_protocol;
      local_addr.sll_ifindex = suppl_intf->if_index;

      ret = bind(sock, (struct sockaddr *)&local_addr,
                 sizeof(struct sockaddr_ll));
      if (ret == QAPI_ERROR) {
        warn_printf("%s %d\n", __FUNCTION__, __LINE__);
        closesocket(wl8021x_intf->eapol_sock);
        continue;
      }
    }
  }
}

static void wlan_8021x_mode_exit_cb(unsigned int dest_mode_id, void *cb_cxt) {
  info_printf("%s\n", __FUNCTION__);
}

static void wlan_8021x_associated_event(void *eloop_ctx, void *timeout_ctx) {
  wlan_8021x_intf_t *wl8021x_intf = (wlan_8021x_intf_t *)timeout_ctx;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  info_printf("%s +++\n", __FUNCTION__);
  if (wlan_lib_auth_is_8021x(suppl_intf->auth_mode) == true) {
    wlan_8021x_initiate_eapol(wl8021x_intf);
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, false);
    eapol_sm_notify_portValid(wl8021x_intf->eapol, false);
    /* start 802.1x authenticating */
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, true);
    /*
     * The driver will take care of RSN 4-way handshake, so we need
     * to allow EAPOL supplicant to complete its work without
     * waiting for WPA supplicant.
     */
    eapol_sm_notify_portValid(wl8021x_intf->eapol, true);
    wl8021x_intf->eapol_received = 0;
    WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_AUTHENTICATING;
  }
  info_printf("%s ---\n", __FUNCTION__);
}

static void wlan_8021x_disconnected_event(void *eloop_ctx, void *timeout_ctx) {
  wlan_8021x_intf_t *wl8021x_intf = (wlan_8021x_intf_t *)timeout_ctx;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  info_printf("%s +++\n", __FUNCTION__);
  if ((wlan_lib_auth_is_8021x(suppl_intf->auth_mode) == true)) {
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, false);
    eapol_sm_notify_portValid(wl8021x_intf->eapol, false);
    eapol_sm_notify_config(wl8021x_intf->eapol, NULL, NULL);
  }
  info_printf("%s ---\n", __FUNCTION__);
}

static void wlan_8021x_rx_eapol_key_event(void *eloop_ctx, void *timeout_ctx) {
  wlan_8021x_intf_t *wl8021x_intf = (wlan_8021x_intf_t *)timeout_ctx;
  info_printf("%s\n", __FUNCTION__);
  wlan_8021x_rx_eapol_key_notify(wl8021x_intf);
}

int wlan_8021x_event_cb(unsigned char device_ID, unsigned int event_ID,
                        void *cb_cxt, void *payload,
                        unsigned int payload_Length) {
  wlan_8021x_intf_t *wl8021x_intf = NULL;
  wlan_8021x_global_t *global = (wlan_8021x_global_t *)cb_cxt;
  unsigned char intf_id = INVALID_INTF_ID;
  suppl_intf_t *suppl_intf;

  info_printf("%s device_ID=%d event_ID=%d cxt=0x%x\n", __FUNCTION__, device_ID,
              event_ID, cb_cxt);
  intf_id =
      suppl_get_intf_id((suppl_global_t *)WL8021X_2_SUPPL(global), device_ID);
  if (intf_id == INVALID_INTF_ID) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }
  wl8021x_intf = &global->wl8021x_intfs[intf_id];
  suppl_intf = (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  // TODO: if 802.1x and not authorized yet
  if ((event_ID == QAPI_WLAN_CONNECT_CB_E) && payload) {
    qapi_WLAN_Connect_Cb_Info_t *cxnInfo =
        (qapi_WLAN_Connect_Cb_Info_t *)(payload);
    if (cxnInfo->value == true) { /* associated event */
      info_printf("%s associated event\n", __FUNCTION__);
      memcpy(suppl_intf->bssid, cxnInfo->mac_Addr, ETH_ALEN);
      eloop_register_timeout(0, 0, wlan_8021x_associated_event, NULL,
                             wl8021x_intf);
    } else if (cxnInfo->value == false) { /* disconnect event */
      info_printf("%s disconnected event\n", __FUNCTION__);
      eloop_register_timeout(0, 0, wlan_8021x_disconnected_event, NULL,
                             wl8021x_intf);
    }
  } else if ((event_ID == QAPI_WLAN_RX_EAPOL_KEY_CB_E) && payload) {
    qapi_WLAN_RxEapolKey_Cb_Info_t *RxEapolKey =
        (qapi_WLAN_RxEapolKey_Cb_Info_t *)payload;
    uint16_t key_info = WPA_GET_BE16(RxEapolKey->keyInfo);
    info_printf("%s RX_EAPOL_KEY type=%d keyinfo=0x%x\n", __FUNCTION__,
                RxEapolKey->descType, key_info);
    wpa_hexdump(MSG_MSGDUMP, "RxEapolKey", RxEapolKey,
                sizeof(qapi_WLAN_RxEapolKey_Cb_Info_t));
    if ((key_info & WPA_KEY_INFO_KEY_TYPE) && !(key_info & WPA_KEY_INFO_MIC)) {
      /* receive M1 */
      eloop_register_timeout(0, 0, wlan_8021x_rx_eapol_key_event, NULL,
                             wl8021x_intf);
      info_printf("%s receive M1\n", __FUNCTION__);
    }
  }

  return QAPI_OK;
}

static void wlan_8021x_eapol_packet_receive(int sock, void *eloop_ctx,
                                            void *sock_ctx) {
  wlan_8021x_intf_t *wl8021x_intf = sock_ctx;
  wlan_8021x_global_t *global = wl8021x_intf->wl8021x_global;
  int eapol_packet_buffer_size =
      global->wl8021x_global_cfg.eapol_packet_buffer_size;
  struct sockaddr from;
  int fromlen = 0;
  int received = 0;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  char *rbuf = os_malloc(eapol_packet_buffer_size);
  if (!rbuf) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }

  os_memzero(&from, sizeof(from));
  fromlen = sizeof(struct sockaddr);

  received = recvfrom(wl8021x_intf->eapol_sock, rbuf, eapol_packet_buffer_size,
                      0, (struct sockaddr *)&from, &fromlen);
  if ((received >= 0) &&
      (wlan_lib_auth_is_8021x(suppl_intf->auth_mode) == true)) {
    struct ether_header *eth = (struct ether_header *)rbuf;
    log_printf("if_name=%s rx src=" MACSTR " len=%d\n", suppl_intf->if_name,
                MAC2STR(eth->ether_shost), received);
    wlan_8021x_rx_eapol(wl8021x_intf, eth->ether_shost,
                        (const unsigned char *)(eth + 1),
                        received - sizeof(struct ether_header));
  } else {
    warn_printf("%s %d drop packet\n", __FUNCTION__, __LINE__);
  }
  os_free(rbuf);
}

static int wlan_8021x_intf_init(wlan_8021x_intf_t *wl8021x_intf) {
  wlan_8021x_global_t *global = wl8021x_intf->wl8021x_global;
  struct sockaddr_ll local_addr;
  int ret = QAPI_ERROR;
  int sock = QAPI_ERROR;
  wlan_8021x_intf_config_t *wl8021x_intf_cfg = &wl8021x_intf->wl8021x_intf_cfg;
  struct eap_peer_config *p_eap_peer_config = &wl8021x_intf_cfg->eap;
  struct eap_method_type *methods = NULL;
  int i = 0;
  int method_total_cnt = global->wl8021x_global_cfg.input_method_cnt + 1;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  log_printf("%s+++ id=%d\n", __FUNCTION__, wl8021x_intf->wl8021x_intf_id);

  p_eap_peer_config->fragment_size = DEFAULT_EAPOL_FRAGMENT_SIZE;

  methods = (struct eap_method_type *)os_zalloc(sizeof(struct eap_method_type) *
                                                method_total_cnt);
  if (!methods) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto method_fail;
  }
  for (i = 0; i < method_total_cnt; i++) {
    methods[i].vendor = EAP_VENDOR_IETF;
    methods[i].method = EAP_TYPE_NONE;
  }
  p_eap_peer_config->eap_methods = methods;
  p_eap_peer_config->phase1 = NULL;

  WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_INIT;
  wl8021x_intf->eapol_received = 0;
  sock = socket(AF_PACKET, SOCK_RAW, global->eapol_protocol);
  if (sock == QAPI_ERROR) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto rx_socket_create_fail;
  }
  wl8021x_intf->eapol_sock = sock;

  // os_memzero(&local_addr, sizeof(local_addr));
  // local_addr.sll_family = AF_PACKET;
  // local_addr.sll_protocol = global->eapol_protocol;
  // local_addr.sll_ifindex = suppl_intf->if_index;

  // ret = bind(sock, (struct sockaddr *)&local_addr, sizeof(struct
  // sockaddr_ll)); if (ret == QAPI_ERROR) {
  //     warn_printf("%s %d\n", __FUNCTION__, __LINE__);
  //     goto rx_socket_bind_fail;
  // }

  ret = eloop_register_read_sock(sock, wlan_8021x_eapol_packet_receive, NULL,
                                 wl8021x_intf);
  if (ret < 0) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto register_sock_fail;
  }

  ret = wlan_8021x_init_eapol(wl8021x_intf);
  if (ret == QAPI_ERROR) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto init_eapol_fail;
  }

  log_printf("wl8021x_intf id=%d if_index=%d name=%s devid=%d mac=" MACSTR
              "\n",
              suppl_intf->if_index, suppl_intf->if_name, suppl_intf->dev_id,
              MAC2STR(suppl_intf->if_mac));
  return QAPI_OK;

init_eapol_fail:
  wlan_8021x_deinit_eapol(wl8021x_intf);

register_sock_fail:
  eloop_unregister_read_sock(sock);

// rx_socket_bind_fail:
//   closesocket(sock);
//   wl8021x_intf->eapol_sock = 0;

rx_socket_create_fail:
  os_free(methods);
  p_eap_peer_config->eap_methods = NULL;

method_fail:
  return QAPI_ERROR;
}

static void wlan_8021x_intf_exit(wlan_8021x_intf_t *wl8021x_intf) {
  wlan_8021x_intf_config_t *wl8021x_intf_cfg = &wl8021x_intf->wl8021x_intf_cfg;
  struct eap_peer_config *p_eap_peer_config = &wl8021x_intf_cfg->eap;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  info_printf("%s+++ id=%d\n", __FUNCTION__, wl8021x_intf->wl8021x_intf_id);
  if (wlan_lib_auth_is_8021x(suppl_intf->auth_mode) == true) {
    /* If authenticating, stop */
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, false);
    eapol_sm_notify_portValid(wl8021x_intf->eapol, false);
    eapol_sm_notify_config(wl8021x_intf->eapol, NULL, NULL);
    WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_INIT;
  }
  wlan_8021x_deinit_eapol(wl8021x_intf);
  if (wl8021x_intf->eapol_sock) {
    eloop_unregister_read_sock(wl8021x_intf->eapol_sock);
    closesocket(wl8021x_intf->eapol_sock);
    wl8021x_intf->eapol_sock = 0;
  }
  os_free(p_eap_peer_config->eap_methods);
  p_eap_peer_config->eap_methods = NULL;
  p_eap_peer_config->phase2 = NULL;

  os_free(p_eap_peer_config->identity);
  os_free(p_eap_peer_config->anonymous_identity);
  os_free(p_eap_peer_config->password);
  os_free(p_eap_peer_config->cert.client_cert);
  os_free(p_eap_peer_config->cert.private_key);
  os_free(p_eap_peer_config->cert.private_key_passwd);
  p_eap_peer_config->identity = NULL;
  p_eap_peer_config->anonymous_identity = NULL;
  p_eap_peer_config->password = NULL;
  p_eap_peer_config->cert.client_cert = NULL;
  p_eap_peer_config->cert.private_key = NULL;
  p_eap_peer_config->cert.private_key_passwd = NULL;
  // p_eap_peer_config->no_server_auth = 0;
  info_printf("%s ---\n", __FUNCTION__);
}

int wlan_8021x_set_auth_mode(uint16_t auth_mode) {
  unsigned char intf_id = INVALID_INTF_ID;
  wlan_8021x_intf_t *wl8021x_intf = NULL;
  suppl_intf_t *suppl_intf;

  intf_id =
      suppl_get_intf_id((suppl_global_t *)WL8021X_2_SUPPL(g_wl8021x_global), 0);
  if (intf_id == INVALID_INTF_ID) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }
  wl8021x_intf = &g_wl8021x_global->wl8021x_intfs[intf_id];

  suppl_intf = (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);
  suppl_intf->auth_mode = auth_mode;

  return QAPI_OK;
}

static int wlan_8021x_set_method(wlan_8021x_intf_t *wl8021x_intf,
                                 qapi_WLAN_8021X_Method_e *data) {
  qapi_WLAN_8021X_Method_e method = *data;
  struct eap_peer_config *p_eap_peer_config =
      &wl8021x_intf->wl8021x_intf_cfg.eap;
  struct eap_method_type *p_method = &p_eap_peer_config->eap_methods[0];

  info_printf("%s method=%d\n", __FUNCTION__, method);

  switch (method) {
  case QAPI_WLAN_8021X_METHOD_EAP_TLS_E:
    p_method->method = EAP_TYPE_TLS;
    //p_eap_peer_config->phase1 = "tls_disable_time_checks=1";
    p_eap_peer_config->phase1 = NULL;
    p_eap_peer_config->phase2 = NULL;
    info_printf("%s EAP_TYPE_TLS\n", __FUNCTION__);
    break;
  case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MSCHAPV2_E:
    p_method->method = EAP_TYPE_TTLS;
    p_eap_peer_config->phase1 = NULL;
    p_eap_peer_config->phase2 = "autheap=MSCHAPV2";
    info_printf("%s EAP_TTLS_MSCHAPV2\n", __FUNCTION__);
    break;
  case QAPI_WLAN_8021X_METHOD_EAP_PEAP_MSCHAPV2_E:
    p_method->method = EAP_TYPE_PEAP;
    p_eap_peer_config->phase1 = NULL;
    p_eap_peer_config->phase2 = "auth=MSCHAPV2";
    info_printf("%s EAP_PEAP_MSCHAPV2\n", __FUNCTION__);
    break;
  case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MD5_E:
    p_method->method = EAP_TYPE_TTLS;
    p_eap_peer_config->phase1 = NULL;
    p_eap_peer_config->phase2 = "autheap=MD5"; // default to be EAP
    info_printf("%s EAP_TTLS_EAP\n", __FUNCTION__);
    break;
  default:
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }
  p_method->vendor = EAP_VENDOR_IETF;
  return QAPI_OK;
}

qapi_Status_t wlan_8021x_set_param(uint8_t device_ID, uint16_t param_ID,
                                   const void *data, uint32_t length,
                                   void *cb_cxt) {
  int ret = QAPI_ERROR;
  struct eap_peer_config *p_eap_peer_config = NULL;
  char *str = NULL;
  wlan_8021x_intf_t *wl8021x_intf = NULL;
  unsigned char intf_id = INVALID_INTF_ID;
  wlan_8021x_global_t *global = (wlan_8021x_global_t *)cb_cxt;

  info_printf("%s devid=%d paramid=%d\n", __FUNCTION__, device_ID, param_ID);
  if (!global || !data ||
      !ID_IN_RANGE(param_ID, __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_START,
                   __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_END)) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }

  intf_id = suppl_get_intf_id((suppl_global_t *)WL8021X_2_SUPPL(global), 0);
  if (intf_id == INVALID_INTF_ID) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }
  wl8021x_intf = &global->wl8021x_intfs[intf_id];
  p_eap_peer_config = &wl8021x_intf->wl8021x_intf_cfg.eap;

  switch (param_ID) {
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD:
    ret = wlan_8021x_set_method(wl8021x_intf, (qapi_WLAN_8021X_Method_e *)data);
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_IDENTITY:
    str = (char *)data;
    info_printf("%s default id=%s\n", __FUNCTION__, str);
    if (*str) {
      os_free(p_eap_peer_config->anonymous_identity);
      p_eap_peer_config->anonymous_identity = (uint8_t *)os_strdup(str);
      p_eap_peer_config->anonymous_identity_len = os_strlen(str);
      ret = QAPI_OK;
    }
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_USERNAME:
    str = (char *)data;
    info_printf("%s username=%s\n", __FUNCTION__, str);
    if (*str) {
      os_free(p_eap_peer_config->identity);
      p_eap_peer_config->identity = (uint8_t *)os_strdup(str);
      p_eap_peer_config->identity_len = os_strlen(str);
      ret = QAPI_OK;
    }
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PASSWORD:
    str = (char *)data;
    info_printf("%s password=%s\n", __FUNCTION__, str);
    if (*str) {
      os_free(p_eap_peer_config->password);
      p_eap_peer_config->password = (uint8_t *)os_strdup(str);
      p_eap_peer_config->password_len = os_strlen(str);
      ret = QAPI_OK;
    }
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CER:
    str = (char *)data;
    info_printf("%s CA cert=%s\n", __FUNCTION__, str);
    if (*str) {
      os_free(p_eap_peer_config->cert.ca_cert);
      p_eap_peer_config->cert.ca_cert = os_strdup(str);
      ret = QAPI_OK;
    }
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CER:
    str = (char *)data;
    info_printf("%s cert=%s\n", __FUNCTION__, str);
    if (*str) {
      os_free(p_eap_peer_config->cert.client_cert);
      p_eap_peer_config->cert.client_cert = os_strdup(str);
      ret = QAPI_OK;
    }
    break;
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY: {
    qapi_WLAN_Security_8021x_Private_Key_t *key =
        (qapi_WLAN_Security_8021x_Private_Key_t *)data;
    warn_printf("%s set key=%s key_passwd=%s\n", __FUNCTION__,
                key->Private_Key_filename ? key->Private_Key_filename : "NULL",
                key->Private_Key_Password ? key->Private_Key_Password : "NULL");
    os_free(p_eap_peer_config->cert.private_key);
    os_free(p_eap_peer_config->cert.private_key_passwd);
    p_eap_peer_config->cert.private_key = os_strdup(key->Private_Key_filename);
    p_eap_peer_config->cert.private_key_passwd =
        os_strdup(key->Private_Key_Password);
    ret = QAPI_OK;
    break;
  }
  case __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_NO_SERVER_AUTH: {
    // p_eap_peer_config->no_server_auth = *(uint32_t *)data;
    ret = QAPI_OK;
    break;
  }
  default:
    break;
  }

  return ret;
}

int wlan_8021x_init(void) {
  wlan_8021x_global_t *wl8021x_global = NULL;
  wlan_8021x_global_config_t *wl8021x_global_cfg = NULL;
  wlan_8021x_intf_t *wl8021x_intf = NULL;
  int ret = QAPI_OK;
  int intfs_cnt = 0;
  int i;

  log_printf("%s\n", __FUNCTION__);

  if (!g_suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto exit;
  }

  ret = wlan_8021x_eap_register_methods();
  if (ret) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto eap_register_fail;
  }

  wl8021x_global = os_malloc(sizeof(wlan_8021x_global_t));
  if (!wl8021x_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto eap_register_fail;
  }
  os_memzero(wl8021x_global, sizeof(wlan_8021x_global_t));
  WL8021X_2_WLANCXT(wl8021x_global) = gp_wlan_qapi_cxt;
  WL8021X_2_SUPPL(wl8021x_global) = g_suppl_global;
  wl8021x_global->eapol_protocol = ETHPROTO_EAP;

  wl8021x_global_cfg = &wl8021x_global->wl8021x_global_cfg;
  wl8021x_global_cfg->eapol_version = DEFAULT_EAPOL_VERSION;
  wl8021x_global_cfg->fast_reauth = DEFAULT_FASR_REAUTH;
  wl8021x_global_cfg->eap_workaround = DEFAULT_EAP_WORKAROUND;
  wl8021x_global_cfg->eapol_packet_buffer_size = CFG_EAPOL_PACKET_BUFFER_SIZE;
  wl8021x_global_cfg->input_method_cnt = CONFIG_INPUT_METHOD_CNT;

  intfs_cnt = g_suppl_global->intfs_cnt;
  wl8021x_global->intfs_cnt = intfs_cnt;

  wl8021x_global->wl8021x_intfs =
      os_malloc(sizeof(wlan_8021x_intf_t) * intfs_cnt);

  if (!wl8021x_global->wl8021x_intfs) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto malloc_fail1;
  }
  os_memzero(wl8021x_global->wl8021x_intfs,
             sizeof(wlan_8021x_intf_t) * intfs_cnt);

  /* preset before initialization */
  for (i = 0; i < intfs_cnt; i++) {
    wl8021x_intf = &wl8021x_global->wl8021x_intfs[i];
    wl8021x_intf->wl8021x_global = wl8021x_global;
    wl8021x_intf->wl8021x_intf_id = i;
    WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf) = g_suppl_global->suppl_intfs[i];
    if (wlan_8021x_intf_init(wl8021x_intf) != QAPI_OK) {
      goto intf_init_fail;
    }
  }

  g_wl8021x_global = wl8021x_global;
  return QAPI_OK;

// event_reg_fail:
  // wlan_lib_deregister_cbs(&g_wl8021x_cblist);

intf_init_fail:
  for (i = 0; i < intfs_cnt; i++) {
    wl8021x_intf = &wl8021x_global->wl8021x_intfs[i];
    wlan_8021x_intf_exit(wl8021x_intf);
    wl8021x_intf->wl8021x_global = NULL;
  }

  os_free(wl8021x_global->wl8021x_intfs);
  wl8021x_global->wl8021x_intfs = NULL;

malloc_fail1:
  os_free(wl8021x_global);

eap_register_fail:
  wlan_8021x_eap_unregister_methods();

exit:
  return -1;
}

void wlan_8021x_exit(void) {
  wlan_8021x_global_t *wl8021x_global = g_wl8021x_global;

  info_printf("%s+++\n", __FUNCTION__);

  if (!wl8021x_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }

  warn_printf("%s %d\n", __FUNCTION__, __LINE__);
  // wlan_lib_deregister_cbs(&g_wl8021x_cblist);
  warn_printf("%s %d\n", __FUNCTION__, __LINE__);

  if (wl8021x_global->wl8021x_intfs) {
    int i;
    wlan_8021x_intf_t *wl8021x_intf = NULL;
    int intfs_cnt = wl8021x_global->intfs_cnt;

    for (i = 0; i < intfs_cnt; i++) {
      wl8021x_intf = &wl8021x_global->wl8021x_intfs[i];
      wlan_8021x_intf_exit(wl8021x_intf);
      wl8021x_intf->wl8021x_global = NULL;
      WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf) = NULL;
    }
    os_free(wl8021x_global->wl8021x_intfs);
    wl8021x_global->wl8021x_intfs = NULL;
  }
  WL8021X_2_WLANCXT(wl8021x_global) = NULL;
  os_free(wl8021x_global);
  g_wl8021x_global = NULL;
  wlan_8021x_eap_unregister_methods();
  info_printf("%s---\n", __FUNCTION__);
}
