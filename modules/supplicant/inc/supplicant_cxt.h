/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SUPPLICANT_CXT_H_
#define _SUPPLICANT_CXT_H_

#include "includes.h"
#include "common.h"
#include "qurt_internal.h"

uint64 timestamp_ms64(void);
uint32 timestamp_ms32(void);
uint32 tick2ms32(uint32 tick);

typedef struct {
  unsigned int reserved;
  char supplicant_version;
} suppl_global_config_t;

typedef struct {
  unsigned int reserved;
} suppl_intf_config_t;

struct suppl_global_s;

typedef enum {
  SUPPL_DISCONNECTED = 0,
  SUPPL_AUTHENTICATING,
  SUPPL_ASSOCIATING,
  SUPPL_ASSOCIATED,
  SUPPL_COMPLETED,
} SUPPLICANT_STATE_e;

#define PMK_LEN_MAX 48
#define PMK_LEN 32
#define PMKID_LEN 16

#define INVALID_INTF_ID (0xFF)

typedef struct suppl_intf_s {
  struct suppl_global_s *suppl_global;
  suppl_intf_config_t suppl_intf_cfg;
  unsigned char intf_id;
  SUPPLICANT_STATE_e suppl_intf_state;
  char *if_name;
  unsigned int if_index;
  unsigned char if_mac[ETH_ALEN];
  unsigned int if_mac_len;
  char *dev_name;
  unsigned char dev_id;
  unsigned short auth_mode;
  unsigned char bssid[ETH_ALEN];
  unsigned char pmk[PMK_LEN_MAX];
  unsigned int pmk_len;
  unsigned char pmkid[PMKID_LEN];
} suppl_intf_t;

#define MAX_INTERFACES_CNT 2
#define WLAN_SUPPLICANT_INTERFACE_ID 0

typedef struct suppl_global_s {
  void *suppl_priv;
  suppl_global_config_t suppl_global_cfg;
  suppl_intf_t *suppl_intfs[MAX_INTERFACES_CNT];
  int intfs_cnt;
  int max_intfs_cnt;
  // qosal_task_handle       task_handle;
  // qosal_event_handle      destroy_event;
  TaskHandle_t task_handle;
  //int destroy_event;
} suppl_global_t;

#define SUPPL_2_WLANCXT(global) (((suppl_global_t *)(global))->suppl_priv)
#define SUPPL_INTF_STATE(suppl_intf)                                           \
  (((suppl_intf_t *)suppl_intf)->suppl_intf_state)

extern suppl_global_t *g_suppl_global;

extern int wlan_set_pmk(unsigned char dev_id, unsigned char *pmk,
                        unsigned int pmk_len);
extern int wlan_set_pmkid(unsigned char dev_id, unsigned char *pmkid,
                          unsigned char *bssid, bool enable);
extern int wlan_generate_pmkid(const u8 *pmk, size_t pmk_len,
                               const u8 *auth_addr, const u8 *suppl_addr,
                               u8 *pmkid);
extern unsigned char suppl_get_intf_id(suppl_global_t *suppl_global,
                                       unsigned char device_ID);

#endif /* _SUPPLICANT_CXT_H_ */
