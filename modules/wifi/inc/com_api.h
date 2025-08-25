/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _CM_API_H_
#define _CM_API_H_

#include "wlan_dev.h"

typedef enum {
    CM_CONNECT_WITHOUT_SCAN = 0x0001,
    CM_CONNECT_DO_WPA_OFFLOAD = 0x0002,
    CM_CONNECT_DO_NOT_DEAUTH = 0x0004,
    CM_CONNECT_DO_WPS_OFFLOAD = 0x0008,
    CM_CONNECT_ENABLE_WPS_OFFLOAD = 0x0010,
    CM_CONNECT_IGNORE_BSSID_HINT = 0x0020,
    CM_CONNECT_STAY_AWAKE = 0x0040,
    CM_CONNECT_ENABLE_RMF = 0x0080,
} CM_CONNECT_TYPE;

/* Governs configuration for all possible sources of handoff triggers */
typedef enum {
    BSS_AGE_METRIC = 0x0001,
    CONNECTION_STATE_METRIC = 0x0002,
    RSSI_AVG_METRIC = 0x0004,
    PMKID_METRIC = 0x0008,
    HOST_REQUEST = 0x0010,
    BMISS_EVENT = 0x0020,
    DISCONNECT_EVENT = 0x0040,
    LOW_RSSI_EVENT = 0x0080,
    BSS_CHANNEL_CHANGE = 0x0100,
    PERIODIC_SEARCH_COMPLETE = 0x0200,
    RECONNECT_EVENT = 0x0400,
    CONNECT_REQUEST = 0x0800,
} HANDOFF_TRIGGER_SOURCE;
#ifdef NT_FN_DEBUG_STATS
typedef struct com_stats_s {
    uint32_t roam_count;
    uint32_t connect_count;
    uint32_t disconnect_count;
    uint32_t roam_start;
    uint32_t roam_duration;
} COM_STATS;
#endif  // NT_FN_DEBUG_STATS

enum cserv_disc_reason {
    CSERV_DISC_DEFAULT = (0x100 | CSERV_DISCONNECT),
    CSERV_DISC_NOROAM = (0x200 | CSERV_DISCONNECT),
    CSERV_DISC_RECONN = (0x400 | CSERV_DISCONNECT),
};

typedef enum {
    CM_STATE_IDLE = 0,
    CM_STATE_CONNECTED = 0x01,         /* Connected or not */
    CM_STATE_AT_HOME = 0x02,           /* At or off home channel */
    CM_STATE_ROAMING = 0x04,           /* Handoff in progress or not */
    CM_STATE_CONNECTINPROGRESS = 0x08, /* Connection in progress or not */
    CM_STATE_DICONNECTING = 0x10,
    CM_STATE_CONNECTREQUESTINPROGRESS = 0x20, /* Connection request in progress  */
} CM_CONNECTION_STATE;

typedef enum {
    CM_REASSOC_MODE_ID = 0,
    CM_CONNECT_TYPE_ID,
    CM_VALID_TRIGGER_SOURCE_ID,
    CM_CONNECT_TIMEOUT_ID,
} CM_CONFIG_ID;

#define CM_SET_REASSOC_MODE(dev, mode)                   \
    do {                                                 \
        uint32_t config = (mode);                        \
        cm_set_config(dev, CM_REASSOC_MODE_ID, &config); \
    } while (0);
#define CM_GET_REASSOC_MODE(dev, mode)                           \
    do {                                                         \
        uint32_t config;                                         \
        cm_get_config(dev, CM_REASSOC_MODE_ID, (void *)&config); \
        mode = *((uint8_t *)&config);                            \
    } while (0);

#define CM_SET_CONNECT_TYPE(dev, type)                           \
    do {                                                         \
        uint32_t config = (type);                                \
        cm_set_config(dev, CM_CONNECT_TYPE_ID, (void *)&config); \
    } while (0);
#define CM_GET_CONNECT_TYPE(dev, type)                   \
    do {                                                 \
        cm_get_config(dev, CM_CONNECT_TYPE_ID, &(type)); \
    } while (0);

#define CM_SET_VALID_TRIGGER_SOURCE(dev, source)                 \
    do {                                                         \
        uint32_t config = (source);                              \
        cm_set_config(dev, CM_VALID_TRIGGER_SOURCE_ID, &config); \
    } while (0);
#define CM_GET_VALID_TRIGGER_SOURCE(dev, source)                           \
    do {                                                                   \
        cm_get_config(dev, CM_VALID_TRIGGER_SOURCE_ID, (void *)&(source)); \
    } while (0);

#define CM_SET_CONNECT_TIMEOUT(dev, timeout)                \
    do {                                                    \
        uint32_t config = ((timeout)*1000);                 \
        cm_set_config(dev, CM_CONNECT_TIMEOUT_ID, &config); \
    } while (0);
#define CM_GET_CONNECT_TIMEOUT(dev, source)                           \
    do {                                                              \
        cm_get_config(dev, CM_CONNECT_TIMEOUT_ID, (void *)&(source)); \
    } while (0);

#define CM_SET_STATE_DISCONNECTING(dev)                \
    do {                                               \
        CM_STRUCT *pcm = (CM_STRUCT *)dev->pcm_struct; \
        pcm->conn_state |= CM_STATE_DICONNECTING;      \
    } while (0);

#define CM_DEVICE_CONNECTED(dev)     ((cm_get_conn_state(dev)) & CM_STATE_CONNECTED)
#define CM_DEVICE_DISCONNECTED(dev)  (!CM_DEVICE_CONNECTED(dev))
#define CM_DEVICE_DISCONNECTING(dev) ((cm_get_conn_state(dev)) & CM_STATE_DICONNECTING)

void *cm_init(struct devh_s *dev);
void cm_deinit(void *cm_inst);
void cm_connect_cmd(struct devh_s *dev, conn_profile_t *prof, uint8_t channel_hint);
nt_status_t cm_connect_request(struct devh_s *dev, bss_t *bss);
void cm_connect_request_cb(void *arg, nt_status_t status);
void cm_connect_event(struct devh_s *dev, bss_t *bss);
void cm_assoc_request_info_notify(struct devh_s *dev, bss_t *bss, uint8_t *assocReq, int32_t assocReqLen,
                                  int frameType);
void cm_disconnect_cmd(struct devh_s *dev, conn_profile_t *prof);
void cm_disconnect_event(struct devh_s *dev, bss_t *bss, uint32_t reason, uint32_t protoReasonStatus);
void cm_connect_timeout(TimerHandle_t thandle);
void cm_initiate_handoff(struct devh_s *dev, uint32_t scan_type);
void cm_execute_handoff(void *arg, nt_status_t status);
nt_status_t cm_execute_final_handoff(devh_t *dev, void *arg);
void cm_reconnect_request(struct devh_s *dev);
#ifdef NT_FN_DEBUG_STATS
COM_STATS *cm_get_statistics(struct devh_s *);
#endif  // NT_FN_DEBUG_STATS
void cm_clear_statistics(struct devh_s *);
uint8_t cm_get_conn_state(struct devh_s *);
void cm_set_conn_state(devh_t *dev, uint16_t state);
struct bss_s *cm_get_current_bss(struct devh_s *);
struct channel *cm_get_home_channel(struct devh_s *);
void cm_set_config(struct devh_s *, CM_CONFIG_ID id, void *config);
uint32_t cm_get_config(struct devh_s *, CM_CONFIG_ID id, void *config);
uint8_t cm_wpa_offload_enabled(struct devh_s *dev);
uint16_t form_eapol_frame(void *pdev, uint8_t *fr_hdr, uint8_t *peer, uint8_t rekey_flag);
void nt_cm_print_conn_info(dev_common_t *pDevCmn);
#ifdef CONFIG_CHANNEL_SCHEDULER
/**
 * This function is called when the first my beacon is received, to create the channel
 * request for low and high priority request.
 *
 * @param dev: device structure
 * @return : This function does not reutrn anyting
 */
void cm_reconfig_connected_channel_operation(devh_t *dev);
#endif
#endif /* _CM_API_H_ */
