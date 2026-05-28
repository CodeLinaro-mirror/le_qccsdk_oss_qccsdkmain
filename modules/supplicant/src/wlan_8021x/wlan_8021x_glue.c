/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common.h"

#include "common/eapol_common.h"
#include "eap_peer/eap_methods.h"
#include "eapol_supp/eapol_supp_sm.h"

#include "supplicant_cxt.h"

#include "crypto/sha1.h"
#include "printfext.h"
#include "qapi_status.h"
#include "qapi_wlan_base.h"
#include "sockets.h"
#include "wlan_8021x_cfg.h"
#include "wlan_8021x_cxt.h"

/**
 * wlan_generate_pmkid - Calculate PMK identifier
 * @pmk: Pairwise master key
 * @pmk_len: Length of pmk in bytes
 * @auth_addr: Authenticator address
 * @suppl_addr: Supplicant address
 * @pmkid: Buffer for PMKID
 *
 * IEEE Std 802.11i-2004 - 8.5.1.2 Pairwise key hierarchy
 * PMKID = HMAC-SHA1-128(PMK, "PMK Name" || AA || SPA)
 */
int wlan_generate_pmkid(const u8 *pmk, size_t pmk_len, const u8 *auth_addr,
                        const u8 *suppl_addr, u8 *pmkid) {
  char *title = "PMK Name";
  const u8 *addr[3];
  const size_t len[3] = {8, ETH_ALEN, ETH_ALEN};
  unsigned char hash[SHA1_MAC_LEN];
  int iRet;

  addr[0] = (u8 *)title;
  addr[1] = auth_addr;
  addr[2] = suppl_addr;

  iRet = hmac_sha1_vector(pmk, pmk_len, 3, addr, len, hash);
  if (iRet != 0)
    return iRet;

  os_memcpy(pmkid, hash, __QAPI_WLAN_PMKID_LEN);
  return 0;
}

int wlan_8021x_eap_register_methods(void) {
  int ret = QAPI_OK;

  log_printf("%s\n", __FUNCTION__);
#ifdef EAP_MD5
  if (ret == 0) {
    ret = eap_peer_md5_register();
  }
#endif /* EAP_MD5 */

#ifdef EAP_TLS
  if (ret == 0) {
    ret = eap_peer_tls_register();
  }
#endif /* EAP_TLS */

#ifdef EAP_UNAUTH_TLS
  if (ret == 0) {
    ret = eap_peer_unauth_tls_register();
  }
#endif /* EAP_UNAUTH_TLS */

#ifdef EAP_MSCHAPv2
  if (ret == 0) {
    ret = eap_peer_mschapv2_register();
  }
#endif /* EAP_MSCHAPv2 */

#ifdef EAP_PEAP
  if (ret == 0) {
    ret = eap_peer_peap_register();
  }
#endif /* EAP_PEAP */

#ifdef EAP_TTLS
  if (ret == 0) {
    ret = eap_peer_ttls_register();
  }
#endif /* EAP_TTLS */

  return ret;
}

static u8 *wlan_8021x_alloc_eapol(const wlan_8021x_intf_t *wl8021x_intf,
                                  u8 type, const void *data, u16 data_len,
                                  size_t *msg_len, void **data_pos) {
  struct ieee802_1x_hdr *hdr = NULL;
  struct ethhdr *eth = NULL;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  log_printf("%s\n", __FUNCTION__);
  *msg_len = sizeof(*eth) + sizeof(*hdr) + data_len;
  eth = os_malloc(*msg_len);
  if (eth == NULL) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return NULL;
  }
  os_memcpy(eth->h_dest, suppl_intf->bssid, ETH_ALEN);
  os_memcpy(eth->h_source, suppl_intf->if_mac, ETH_ALEN);
  eth->h_proto = htons(ETH_P_EAPOL);

  hdr = (struct ieee802_1x_hdr *)(eth + 1);
  hdr->version = wl8021x_intf->wl8021x_global->wl8021x_global_cfg.eapol_version;
  hdr->type = type;
  hdr->length = host_to_be16(data_len);

  if (data)
    os_memcpy(hdr + 1, data, data_len);
  else
    os_memset(hdr + 1, 0, data_len);

  if (data_pos)
    *data_pos = hdr + 1;

  return (u8 *)eth;
}

static int wlan_8021x_ether_send(wlan_8021x_intf_t *wl8021x_intf,
                                 const u8 *dest, u16 proto, const u8 *buf,
                                 size_t len) {
  int ret;
  int sock = socket(AF_PACKET, SOCK_RAW, ETHPROTO_EAP);
  if (sock == QAPI_ERROR) {
    warn_printf("ERROR: Failed to create socket\n");
    return QAPI_ERROR;
  }
  ret = sendto(sock, (char *)buf, len, 0, NULL, 0);
  if (ret < 0) {
    warn_printf("%s raw pkt send fail: %d\n", __FUNCTION__, ret);
  }
  closesocket(sock);
  return ret;
}

static int wlan_8021x_eapol_send(void *ctx, int type, const u8 *buf,
                                 size_t len) {
  struct wlan_8021x_intf_s *wl8021x_intf = (struct wlan_8021x_intf_s *)ctx;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);
  u8 *msg, *dst;
  size_t msglen;
  int res;

  log_printf("%s\n", __FUNCTION__);
  msg = wlan_8021x_alloc_eapol(wl8021x_intf, type, buf, len, &msglen, NULL);
  if (msg == NULL) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }

  dst = suppl_intf->bssid;
  log_printf("TX EAPOL: dst=" MACSTR "\n", MAC2STR(dst));
  res = wlan_8021x_ether_send(wl8021x_intf, dst, ETH_P_EAPOL, msg, msglen);
  os_free(msg);

  return res;
}

static void wlan_8021x_notify_eapol_done(void *ctx) {
  info_printf("%s WPA: EAPOL authenticating complete\n", __FUNCTION__);
}

static void wlan_8021x_aborted_cached(void *ctx) {
  info_printf("%s\n", __FUNCTION__);
}

static void wlan_8021x_port_cb(void *ctx, int authorized) {
  info_printf("EAPOL: Supplicant port status: %s\n",
              authorized ? "Authorized" : "Unauthorized");
}

static int wlan_8021x_get_pmk(unsigned char *pmk, unsigned int *p_pmk_len,
                              wlan_8021x_intf_t *wl8021x_intf) {
  int res = QAPI_OK;

  info_printf("%s\n", __FUNCTION__);
  if (wl8021x_intf->eapol) {
    int pmk_len;

    pmk_len = PMK_LEN;
    res = eapol_sm_get_key(wl8021x_intf->eapol, pmk, pmk_len);
    if (!res) {
      *p_pmk_len = pmk_len;
    }
  }
  return res;
}

static void wlan_8021x_eapol_cb(struct eapol_sm *eapol,
                                enum eapol_supp_result result, void *ctx) {
  int i = 0;
  struct wlan_8021x_intf_s *wl8021x_intf = (struct wlan_8021x_intf_s *)ctx;
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  info_printf("%s\n", __FUNCTION__);
  if (result != EAPOL_SUPP_RESULT_SUCCESS) {
    WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_AUTHENTICAT_FAILED;
    return;
  }

  if (!suppl_intf->pmk_len) {
    info_printf("Configure PMK for driver-based RSN 4-way handshake\n");
    wlan_8021x_get_pmk(suppl_intf->pmk, &suppl_intf->pmk_len, wl8021x_intf);
    if (suppl_intf->pmk_len) {
      info_printf("%s set pmk\n", __FUNCTION__);
      for (i = 0; i < suppl_intf->pmk_len; i++) {
                printf("%02x", ((uint8_t *)suppl_intf->pmk)[i]);
      }
      printf("\n");
      wlan_set_pmk(suppl_intf->dev_id, suppl_intf->pmk, suppl_intf->pmk_len);
    }
  }
  WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_AUTHENTICATED;
}

static void wlan_8021x_cert_cb(void *ctx, int depth, const char *subject,
                               const char *altsubject[], int num_altsubject,
                               const char *cert_hash,
                               const struct wpabuf *cert) {
  info_printf("%s\n", __FUNCTION__);
}

static void wlan_8021x_status_cb(void *ctx, const char *status,
                                 const char *parameter) {
  info_printf("%s status=%s parameter=%s\n", __FUNCTION__, status, parameter);
}

static void wlan_8021x_set_anon_id(void *ctx, const u8 *id, size_t len) {
  info_printf("%s\n", __FUNCTION__);
  info_printf("EAP method updated anonymous_identity id=%s len=%d\n",
              (char *)id, len);
}

int wlan_8021x_init_eapol(wlan_8021x_intf_t *wl8021x_intf) {
  struct eapol_ctx *ctx;

  log_printf("%s\n", __FUNCTION__);
  ctx = os_zalloc(sizeof(*ctx));
  if (ctx == NULL) {
    warn_printf("Failed to allocate EAPOL context\n");
    return QAPI_ERROR;
  }
  ctx->ctx = wl8021x_intf;
  ctx->msg_ctx = wl8021x_intf;
  ctx->eapol_send_ctx = wl8021x_intf;
  ctx->preauth = 0;
  ctx->eapol_done_cb = wlan_8021x_notify_eapol_done;
  ctx->eapol_send = wlan_8021x_eapol_send;
  ctx->aborted_cached = wlan_8021x_aborted_cached;
  ctx->opensc_engine_path = NULL;
  ctx->pkcs11_engine_path = NULL;
  ctx->pkcs11_module_path = NULL;
  ctx->openssl_ciphers = NULL;
  ctx->port_cb = wlan_8021x_port_cb;
  ctx->cb = wlan_8021x_eapol_cb;
  ctx->cert_cb = wlan_8021x_cert_cb;
  ctx->cert_in_cb = 1;
  ctx->status_cb = wlan_8021x_status_cb;
  ctx->set_anon_id = wlan_8021x_set_anon_id;
  ctx->cb_ctx = wl8021x_intf;
  wl8021x_intf->eapol = eapol_sm_init(ctx);
  return QAPI_OK;
}

void wlan_8021x_deinit_eapol(wlan_8021x_intf_t *wl8021x_intf) {
  info_printf("%s\n", __FUNCTION__);
  eapol_sm_deinit(wl8021x_intf->eapol);
  wl8021x_intf->eapol = NULL;
}

void wlan_8021x_initiate_eapol(wlan_8021x_intf_t *wl8021x_intf) {
  wlan_8021x_global_config_t *wl8021x_global_cfg =
      &wl8021x_intf->wl8021x_global->wl8021x_global_cfg;
  struct eapol_config eapol_conf;

  info_printf("%s\n", __FUNCTION__);
  os_memzero(&eapol_conf, sizeof(eapol_conf));
  eapol_sm_notify_eap_success(wl8021x_intf->eapol, FALSE);
  eapol_sm_notify_eap_fail(wl8021x_intf->eapol, FALSE);
  eapol_sm_notify_portControl(wl8021x_intf->eapol, Auto);
  eapol_conf.fast_reauth = wl8021x_global_cfg->fast_reauth;
  eapol_conf.workaround = wl8021x_global_cfg->eap_workaround;
  eapol_sm_notify_config(wl8021x_intf->eapol,
                         &wl8021x_intf->wl8021x_intf_cfg.eap, &eapol_conf);
}

void wlan_8021x_rx_eapol_data_notify(wlan_8021x_intf_t *wl8021x_intf) {
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);

  if ((suppl_intf->pmk_len) &&
      (WL8021X_INTF_STATE(wl8021x_intf) != WL8021X_AUTHENTICATED)) {
    /* during authing, if pmk is not set by the last success eap data packet,
     * clear pmk & pmkid
     */
    suppl_intf->pmk_len = 0;
    info_printf("%s clear pmk & pmkid\n", __FUNCTION__);
    os_memzero(suppl_intf->pmkid, PMKID_LEN);
    wlan_set_pmkid(suppl_intf->dev_id, NULL, suppl_intf->bssid, false);
  }

#ifdef CONFIG_ENABLE_REAUTH
  if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_PMK_CACHED) {
    // it is reauth
    WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_AUTHENTICATING;
  }
#endif
}

void wlan_8021x_rx_eapol_key_notify(wlan_8021x_intf_t *wl8021x_intf) {
  suppl_intf_t *suppl_intf =
      (suppl_intf_t *)WL8021X_INTF_2_SUPPL_INTF(wl8021x_intf);
  log_printf("%s +++\n", __FUNCTION__);

  if (wlan_lib_auth_is_8021x(suppl_intf->auth_mode) != true) {
    goto out;
  }

  if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_AUTHENTICATED) {
    /* rx eapol key after auth is completed, then set pmkid */
    int res = 0;

    if (!suppl_intf->pmk_len) {
      res = -1;
      warn_printf("%s pmk not set yet, should not happen\n", __FUNCTION__);
      goto out;
    }

    res = wlan_generate_pmkid(suppl_intf->pmk, suppl_intf->pmk_len,
                              suppl_intf->bssid, suppl_intf->if_mac,
                              suppl_intf->pmkid);
    if (res) {
      warn_printf("%s failed to generate pmkid\n", __FUNCTION__);
      goto out;
    }

    res = wlan_set_pmkid(suppl_intf->dev_id, suppl_intf->pmkid,
                         suppl_intf->bssid, true);
    if (res) {
      warn_printf("%s failed to set pmkid\n", __FUNCTION__);
      goto out;
    }

    WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_PMK_CACHED;
  }

#ifdef CONFIG_ENABLE_PMK_CACHE
  if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_AUTHENTICATING) {
    /* rx eapol key during authing, then it is pmk-caching, force suucess */
    wlan_8021x_force_eapol_success(wl8021x_intf, EAPOL_FORCE_TYPE_PMKID);
    if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_AUTHENTICATED) {
      WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_PMK_CACHED;
    }
  }
#endif

out:
  log_printf("%s ---\n", __FUNCTION__);
}

void wlan_8021x_rx_eapol(wlan_8021x_intf_t *wl8021x_intf,
                         const unsigned char *src_addr,
                         const unsigned char *buf, unsigned int len) {
  int ret = 0;

  log_printf("%s\n", __FUNCTION__);
  wl8021x_intf->eapol_received++;
  ret = eapol_sm_rx_eapol(wl8021x_intf->eapol, src_addr, buf, len,
                          FRAME_ENCRYPTION_UNKNOWN);

  if (!ret) {
    /* rx wpa/rsn eapol key packet.
     * In current fw, this will not happen, since wpa/rsn eapol key packet is
     * filtered by fw.
     */
    log_printf("%s %d rx eapol key packet\n", __FUNCTION__, __LINE__);
    wlan_8021x_rx_eapol_key_notify(wl8021x_intf);
  }

  if (ret == 1) {
    // Rx eapol 8021x data pakcet
    wlan_8021x_rx_eapol_data_notify(wl8021x_intf);
  }
}

#ifdef CONFIG_ENABLE_PMK_CACHE
int wlan_8021x_force_eapol_success(wlan_8021x_intf_t *wl8021x_intf, int type) {
  log_printf("%s type=%d\n", __FUNCTION__, type);
  if (type == EAPOL_FORCE_TYPE_PMKID) {
    eapol_sm_notify_lower_layer_success(wl8021x_intf->eapol, 0);
    eapol_sm_notify_cached(wl8021x_intf->eapol);
  } else {
    warn_printf("%s type=%d unknown\n", __FUNCTION__, type);
    return -1;
  }
  return 0;
}
#endif
