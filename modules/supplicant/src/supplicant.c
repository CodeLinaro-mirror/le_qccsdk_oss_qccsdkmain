/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "supplicant.h"
#include "printfext.h"
#include "qapi/qapi_wlan.h"
#include "qapi_status.h"
#include "supplicant_cxt.h"
#include "wlan_drv.h"

#define DESTORY_EVENT_MASK_TASK 0x01

unsigned int wlan_supplicant_task_prio = 6;
unsigned int wlan_supplicant_task_stack_size = 2048;
static char dev_name[10] = "wlan0";
suppl_global_t *g_suppl_global = NULL;

/* Set pmkid, or clear pmk & pmkid */
int wlan_set_pmkid(unsigned char dev_id, unsigned char *pmkid,
                   unsigned char *bssid, bool enable) {
  qapi_WLAN_Set_PMKID_Params_t set_pmkid_param;
  int ret = 0;

  memset(&set_pmkid_param, 0, sizeof(set_pmkid_param));
  memcpy(set_pmkid_param.bssid, bssid, __QAPI_WLAN_MAC_LEN);
  if (enable == true) {
    set_pmkid_param.enable = QAPI_WLAN_PMKID_ENABLE_E;
    memcpy(set_pmkid_param.pmkid, pmkid, __QAPI_WLAN_PMKID_LEN);
  } else {
    set_pmkid_param.enable = QAPI_WLAN_PMKID_DISABLE_E;
  }
  wpa_hexdump(MSG_MSGDUMP, "set_pmkid_param", &set_pmkid_param,
              sizeof(set_pmkid_param));

  /* When enable, this QAPI only sets pmkid; when disable, this QAPI clears both
   * pmk and pmkid */
  ret = qapi_WLAN_Set_Param(dev_id, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID,
                            &set_pmkid_param, sizeof(set_pmkid_param),
                            QAPI_WLAN_NO_WAIT_E);

  return ret;
}

/* Only sets pmk, cannot clear pmk */
int wlan_set_pmk(unsigned char dev_id, unsigned char *pmk,
                 unsigned int pmk_len) {
  int ret = -1;

  if (!pmk_len) {
    ret = -1;
    warn_printf("%s pmk_len=0\n", __FUNCTION__);
    goto out;
  }
  ret = qapi_WLAN_Set_Param(dev_id, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PMK, pmk, pmk_len,
                            QAPI_WLAN_NO_WAIT_E);
out:
  return ret;
}

unsigned char suppl_get_intf_id(suppl_global_t *suppl_global,
                                unsigned char device_ID) {
  suppl_intf_t *suppl_intf = NULL;
  int i = 0;
  int intfs_cnt = 0;
  unsigned char intf_id = INVALID_INTF_ID;

  if (!suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    intf_id = INVALID_INTF_ID;
    return intf_id;
  }
  intfs_cnt = suppl_global->max_intfs_cnt;

  for (i = 0; i < intfs_cnt; i++) {
    suppl_intf = suppl_global->suppl_intfs[i];
    if (suppl_intf && (suppl_intf->dev_id == device_ID)) {
      intf_id = i;
      break;
    }
  }
  if (i >= intfs_cnt) {
    log_printf("%s %d\n", __FUNCTION__, __LINE__);
    intf_id = INVALID_INTF_ID;
  }
  return intf_id;
}

static void suppl_associated_event(void *eloop_ctx, void *timeout_ctx) {
  suppl_intf_t *suppl_intf = (suppl_intf_t *)timeout_ctx;
  suppl_global_t *global = suppl_intf->suppl_global;
  unsigned char *bssid = NULL;

  // suppl_intf->auth_mode =
  // wlan_lib_get_intf_auth_mode(SUPPL_2_WLANLIB(global), suppl_intf->dev_id);
  // bssid = wlan_lib_get_intf_bssid(SUPPL_2_WLANLIB(global),
  // suppl_intf->dev_id);
  memcpy(suppl_intf->bssid, bssid, ETH_ALEN);
  SUPPL_INTF_STATE(suppl_intf) = SUPPL_ASSOCIATED;
  info_printf("%s\n", __FUNCTION__);
}

static void suppl_handshake_success_event(void *eloop_ctx, void *timeout_ctx) {
  suppl_intf_t *suppl_intf = (suppl_intf_t *)timeout_ctx;
  SUPPL_INTF_STATE(suppl_intf) = SUPPL_COMPLETED;
  info_printf("%s\n", __FUNCTION__);
}

static void suppl_disconnected_event(void *eloop_ctx, void *timeout_ctx) {
  suppl_intf_t *suppl_intf = (suppl_intf_t *)timeout_ctx;
  SUPPL_INTF_STATE(suppl_intf) = SUPPL_DISCONNECTED;
  // suppl_intf->auth_mode = 0;    /* Kept, so other component may use */
  // os_memzero(suppl_intf->bssid, ETH_ALEN);    /* Kept, so other component may
  // use */
  info_printf("%s\n", __FUNCTION__);
}

static int suppl_event_cb(unsigned char device_ID, unsigned int event_ID,
                          void *cb_cxt, void *payload,
                          unsigned int payload_Length) {
  suppl_intf_t *suppl_intf = NULL;
  unsigned char intf_id = INVALID_INTF_ID;
  suppl_global_t *global = (suppl_global_t *)cb_cxt;

  info_printf("%s device_ID=%d event_ID=%d cxt=0x%x\n", __FUNCTION__, device_ID,
              event_ID, cb_cxt);
  intf_id = suppl_get_intf_id(global, device_ID);
  if (intf_id == INVALID_INTF_ID) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return QAPI_ERROR;
  }
  suppl_intf = global->suppl_intfs[intf_id];

  // TODO: if 802.1x and not authorized yet
  if ((event_ID == QAPI_WLAN_CONNECT_CB_E) && payload) {
    qapi_WLAN_Connect_Cb_Info_t *cxnInfo =
        (qapi_WLAN_Connect_Cb_Info_t *)(payload);
    if (cxnInfo->value == true) { /* associated event */
      info_printf("%s associated event\n", __FUNCTION__);
      eloop_register_timeout(0, 0, suppl_associated_event, NULL, suppl_intf);
    } else if (cxnInfo->value ==
               0x10) { /* PEER_FIRST_NODE_JOIN_EVENT, used to RSNA success */
      info_printf("%s 4 way handshake success event\n", __FUNCTION__);
      eloop_register_timeout(0, 0, suppl_handshake_success_event, NULL,
                             suppl_intf);
    } else if (cxnInfo->value == false) { /* disconnect event */
      info_printf("%s disconnected event\n", __FUNCTION__);
      eloop_register_timeout(0, 0, suppl_disconnected_event, NULL, suppl_intf);
    }
  }

  return QAPI_OK;
}

static void suppl_thread_entry(void *arg) {
  suppl_global_t *suppl_global = (suppl_global_t *)arg;

  info_printf("start %s\n", __FUNCTION__);
  eloop_run();
  suppl_global->task_handle = NULL;
  // qosal_set_event(&suppl_global->destroy_event, DESTORY_EVENT_MASK_TASK);
  info_printf("Exit %s\n", __FUNCTION__);
  // qosal_task_destroy(NULL);
  qurt_thread_stop();
}

int wlan_supplicant_add_interface(unsigned char devid) {
  suppl_intf_t *suppl_intf = NULL;
  suppl_global_t *suppl_global = g_suppl_global;
  uint32_t length = IEEE80211_ADDR_LEN;
  uint8_t deviceId = get_active_device();
  int ret;
  int i;

  log_printf("%s id=%d\n", __FUNCTION__, devid);

  if (!suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto exit;
  }

  /* This devid should never be registered */
  if (suppl_get_intf_id(suppl_global, devid) != INVALID_INTF_ID) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto exit;
  }

  suppl_intf = malloc(sizeof(suppl_intf_t));
  if (!suppl_intf) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto malloc_fail1;
  }
  memset(suppl_intf, 0, sizeof(suppl_intf_t));

  suppl_intf->dev_id = devid;
  suppl_intf->dev_name = &dev_name;
  suppl_intf->if_name =
      suppl_intf->dev_name; // dev name is same as suppl_intf name
  suppl_intf->if_index = 0;

  if (QAPI_OK !=
      qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                          __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS,
                          suppl_intf->if_mac, &length)) {
    info_printf("get mac address fail for device %d\n", deviceId);
  }

  suppl_intf->if_mac_len = IEEE80211_ADDR_LEN;
  SUPPL_INTF_STATE(suppl_intf) = SUPPL_DISCONNECTED;
  suppl_intf->auth_mode = 0;

  for (i = 0; i < suppl_global->max_intfs_cnt; i++) {
    if (!suppl_global->suppl_intfs[i]) {
      /* Found empty slot */
      break;
    }
  }
  if (i == suppl_global->max_intfs_cnt) {
    /* No empty slot */
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto malloc_fail1;
  }

  suppl_intf->intf_id = i;
  suppl_intf->suppl_global = suppl_global;

  suppl_global->intfs_cnt++;
  suppl_global->suppl_intfs[i] = suppl_intf;

  return QAPI_OK;

malloc_fail1:
  if (suppl_intf) {
    free(suppl_intf);
  }

exit:
  return QAPI_ERROR;
}

void wlan_supplicant_remove_interface(unsigned char devid) {
  suppl_intf_t *suppl_intf = NULL;
  suppl_global_t *suppl_global = g_suppl_global;
  unsigned char intf_id;

  info_printf("%s+++ id=%d\n", __FUNCTION__, devid);

  if (!suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }

  intf_id = suppl_get_intf_id(suppl_global, devid);
  if (intf_id == INVALID_INTF_ID) {
    info_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }
  suppl_intf = suppl_global->suppl_intfs[intf_id];
  free(suppl_intf);

  suppl_global->intfs_cnt--;
  suppl_global->suppl_intfs[intf_id] = NULL;

  return;
}

int wlan_supplicant_init(void) {
  suppl_global_t *suppl_global = NULL;
  extern wlan_qapi_cxt_t *gp_wlan_qapi_cxt;
  int ret;

  log_printf("%s\n", __FUNCTION__);
  if (g_suppl_global) {
    warn_printf("wlan supplicant already inited\n");
    return QAPI_OK;
  }

  suppl_global = os_malloc(sizeof(suppl_global_t));
  if (!suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto malloc_fail1;
  }
  memset(suppl_global, 0, sizeof(suppl_global_t));
  SUPPL_2_WLANCXT(suppl_global) = gp_wlan_qapi_cxt;
  suppl_global->max_intfs_cnt = MAX_INTERFACES_CNT;

  ret = eloop_init();
  if (ret < 0) {
    goto eloop_init_fail;
  }

  // qosal_create_event(&suppl_global->destroy_event);

  ret = nt_qurt_thread_create(
      suppl_thread_entry, "supplicant", wlan_supplicant_task_stack_size,
      suppl_global, wlan_supplicant_task_prio, &suppl_global->task_handle);
  if (ret == -1) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    goto eloop_init_fail;
  }

  g_suppl_global = suppl_global;

  ret = wlan_supplicant_add_interface(WLAN_SUPPLICANT_INTERFACE_ID);
  if (ret != QAPI_OK) {
    warn_printf("%s failed to add interface\n", __FUNCTION__);
    // clean up
    eloop_terminate();
    eloop_destroy();
    g_suppl_global = NULL;
    free(suppl_global);
    return QAPI_ERROR;
  }

  return QAPI_OK;

eloop_init_fail:
  eloop_destroy();

malloc_fail1:
  free(suppl_global);

  return QAPI_ERROR;
}

void wlan_supplicant_thread_stop(void) {
  suppl_global_t *suppl_global = g_suppl_global;

  info_printf("%s +++\n", __FUNCTION__);

  if (!suppl_global || eloop_terminated()) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }

  eloop_terminate();
  // qosal_wait_for_event(&suppl_global->destroy_event, DESTORY_EVENT_MASK_TASK,
  //     0, NULL, MAX_EVENT_WAIT_TIME_MSEC);
  // qosal_delete_event(&suppl_global->destroy_event);
  eloop_destroy();
  info_printf("%s ---\n", __FUNCTION__);
}

void wlan_supplicant_exit(void) {
  suppl_global_t *suppl_global = g_suppl_global;

  info_printf("%s+++\n", __FUNCTION__);

  if (!suppl_global) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return;
  }

  // wlan_lib_deregister_cbs(&g_suppl_cblist);
  free(suppl_global);
  g_suppl_global = NULL;
  warn_printf("%s---\n", __FUNCTION__);
  return;
}