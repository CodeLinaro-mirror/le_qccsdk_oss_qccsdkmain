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
#include "crypto/sha256.h"
#include "wmi.h"
#include "printfext.h"
#include "qapi_status.h"
#include "qapi_wlan_base.h"
#include "sockets.h"
#include "wlan_8021x.h"
#include "wlan_8021x_cfg.h"
#include "wlan_8021x_cxt.h"
#include "qapi/qapi_lowpower.h"
#include "lowpower_internal.h"
#include "qcc730_os.h"

extern lpr_wmi_t g_lowpower_wmi;

/* Watchdog: if 4-way handshake doesn't complete within this window after
 * EAP-Success, resume BMPS anyway so the system isn't permanently stuck
 * out of power save (e.g. AP never sends M1, MIC failure, etc.). */
#define WLAN_8021X_4WAY_HOLD_WATCHDOG_MS 3000

static void wlan_8021x_eap_leave_ps_hold_watchdog(void *eloop_ctx,
                                                  void *timeout_ctx) {
  wlan_8021x_intf_t *wl8021x_intf = (wlan_8021x_intf_t *)timeout_ctx;
  if (wl8021x_intf->bmps_held_for_eap) {
    warn_printf("8021X: 4-way watchdog fired, resuming BMPS\n");
    wlan_8021x_eap_leave_ps_hold(wl8021x_intf);
  }
}

void wlan_8021x_eap_enter_ps_hold(wlan_8021x_intf_t *wl8021x_intf) {
  if (wl8021x_intf->bmps_held_for_eap) return;
  if (g_lowpower_wmi.bmps_cfg.bmps_enable.enable) {
    qapi_bmps_cfg(0, 0);
    wl8021x_intf->bmps_held_for_eap = true;
    info_printf("8021X: BMPS suspended for EAP\n");
  }
}

void wlan_8021x_eap_leave_ps_hold(wlan_8021x_intf_t *wl8021x_intf) {
  /* Always cancel the watchdog, even if not held - cheap, idempotent. */
  eloop_cancel_timeout(wlan_8021x_eap_leave_ps_hold_watchdog, NULL,
                       wl8021x_intf);
  if (!wl8021x_intf->bmps_held_for_eap) return;
  qapi_bmps_cfg(1, 0);
  wl8021x_intf->bmps_held_for_eap = false;
  info_printf("8021X: BMPS resumed\n");
}

/* Arm the watchdog. Caller has just decided to keep BMPS held until the
 * 4-way handshake completes; if it doesn't, this fires and force-resumes.
 * Skip arming if BMPS isn't actually held (e.g. user hadn't enabled BMPS
 * when association happened) - nothing to release later, and PMKSA-cache
 * fast path also lands here without M3 ever coming back to the host. */
static void wlan_8021x_eap_arm_leave_watchdog(wlan_8021x_intf_t *wl8021x_intf) {
  if (!wl8021x_intf->bmps_held_for_eap) return;
  eloop_cancel_timeout(wlan_8021x_eap_leave_ps_hold_watchdog, NULL,
                       wl8021x_intf);
  eloop_register_timeout(WLAN_8021X_4WAY_HOLD_WATCHDOG_MS / 1000,
                         (WLAN_8021X_4WAY_HOLD_WATCHDOG_MS % 1000) * 1000,
                         wlan_8021x_eap_leave_ps_hold_watchdog, NULL,
                         wl8021x_intf);
}

/**
 * wlan_generate_pmkid - Calculate PMK identifier
 * @pmk: Pairwise master key
 * @pmk_len: Length of pmk in bytes
 * @auth_addr: Authenticator address
 * @suppl_addr: Supplicant address
 * @pmkid: Buffer for PMKID (16 bytes)
 * @auth_mode: AUTH_MODE bitmask, selects the hash:
 *   WPA2 802.1X (SHA1 AKM)        → HMAC-SHA1-128
 *   WPA2-SHA256 / WPA3-SHA256 /
 *   WPA3-Enterprise-only          → HMAC-SHA256-128
 *
 * IEEE 802.11-2020 12.7.1.3: PMKID = Truncate-128(HMAC-Hash(PMK,
 *   "PMK Name" || AA || SPA))
 *
 * WPA3-Enterprise-192bit (CNSA, akm 00-0F-AC:12) wants HMAC-SHA384-128
 * but sha384 isn't linked into this build; that mode is left on the
 * SHA1 fallback branch and is currently unsupported for fast reauth.
 */
int wlan_generate_pmkid(const u8 *pmk, size_t pmk_len, const u8 *auth_addr,
                        const u8 *suppl_addr, u8 *pmkid,
                        unsigned short auth_mode) {
  char *title = "PMK Name";
  const u8 *addr[3];
  const size_t len[3] = {8, ETH_ALEN, ETH_ALEN};
  unsigned char hash[SHA256_MAC_LEN];
  int iRet;

  addr[0] = (u8 *)title;
  addr[1] = auth_addr;
  addr[2] = suppl_addr;

  if (auth_mode & (WMI_WPA2_SHA256_AUTH | WMI_WPA3_SHA256_AUTH |
                   WMI_WPA3_ENTERPRISE_ONLY_AUTH)) {
    iRet = hmac_sha256_vector(pmk, pmk_len, 3, addr, len, hash);
  } else {
    iRet = hmac_sha1_vector(pmk, pmk_len, 3, addr, len, hash);
  }
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
  if (sock < 0) {
    warn_printf("ERROR: Failed to create EAPOL socket: %d\n", sock);
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
    wlan_8021x_eap_leave_ps_hold(wl8021x_intf);
    return;
  }

  /* Always fetch and push the PMK: on reauth the EAP method derives a new
   * key, so the cached pmk_len != 0 guard must not be used. */
  info_printf("Configure PMK for driver-based RSN 4-way handshake\n");
  suppl_intf->pmk_len = 0;
  wlan_8021x_get_pmk(suppl_intf->pmk, &suppl_intf->pmk_len, wl8021x_intf);
  if (suppl_intf->pmk_len) {
    info_printf("%s set pmk\n", __FUNCTION__);
    for (i = 0; i < suppl_intf->pmk_len; i++) {
              printf("%02x", ((uint8_t *)suppl_intf->pmk)[i]);
    }
    printf("\n");
    wlan_set_pmk(suppl_intf->dev_id, suppl_intf->pmk, suppl_intf->pmk_len);
  }
  WL8021X_INTF_STATE(wl8021x_intf) = WL8021X_AUTHENTICATED;
  /* Don't leave PS hold yet: the driver-based RSN 4-way handshake
   * (M1/M2/M3/M4 + PTK/GTK install + PMKID set) hasn't started.  If we
   * resume BMPS here the device may sleep mid-handshake; on wakeup the
   * RX path observed to stay deaf until the next disconnect/reauth.
   * Resume only after rx_eapol_key_notify finishes installing PMKID
   * (state -> PMK_CACHED).  Watchdog covers the case where M1 never
   * arrives. */
  wlan_8021x_eap_arm_leave_watchdog(wl8021x_intf);
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
    wlan_8021x_eap_enter_ps_hold(wl8021x_intf);
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
                              suppl_intf->pmkid, suppl_intf->auth_mode);
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

  if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_PMK_CACHED) {
    /* 4-way handshake done (normal or PMKSA caching) — quiesce the EAPOL
     * SM to stop the residual idleWhile timer tick (set to 60s during EAP
     * INITIALIZE) from waking the chip out of BMPS every second. */
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, false);
    wlan_8021x_eap_leave_ps_hold(wl8021x_intf);
  }

out:
  log_printf("%s ---\n", __FUNCTION__);
}

void wlan_8021x_rx_eapol(wlan_8021x_intf_t *wl8021x_intf,
                         const unsigned char *src_addr,
                         const unsigned char *buf, unsigned int len) {
  int ret = 0;

  log_printf("%s\n", __FUNCTION__);
  wl8021x_intf->eapol_received++;

  if (WL8021X_INTF_STATE(wl8021x_intf) == WL8021X_PMK_CACHED) {
    /* Re-enable EAPOL SM before feeding the packet — it was quiesced after
     * the previous handshake completed. */
    eapol_sm_notify_portEnabled(wl8021x_intf->eapol, true);
    eapol_sm_notify_portValid(wl8021x_intf->eapol, true);
  }

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
