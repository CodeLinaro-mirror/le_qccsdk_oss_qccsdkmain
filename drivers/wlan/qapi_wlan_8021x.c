/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_wlan_8021x.h"
#include "printfext.h"
#include "supplicant.h"
#include "wlan_8021x.h"

qapi_Status_t qapi_WLAN_8021x_Enable(qapi_WLAN_8021x_Enable_e enable) {
  qapi_Status_t ret = QAPI_OK;

  info_printf("%s enable=%d\n", __FUNCTION__, enable);

  if (QAPI_WLAN_8021X_ENABLE_E == enable) {
    if (wlan_8021x_init() == QAPI_OK) {
      ret = QAPI_OK;
    } else {
      ret = QAPI_ERROR;
    }
  } else if (QAPI_WLAN_8021X_DISABLE_E == enable) {
    wlan_supplicant_thread_stop();
    wlan_8021x_exit();
  } else {
    info_printf("%s %d\n", __FUNCTION__, __LINE__);
    ret = QAPI_ERROR;
  }

  return ret;
}