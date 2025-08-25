/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WPS_API_H_
#define _WPS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wps_def.h"

#ifdef ATH_KF
enum {
    IEEE802_1X_TYPE_EAP_PACKET = 0,
    IEEE802_1X_TYPE_EAPOL_START = 1,
    IEEE802_1X_TYPE_EAPOL_LOGOFF = 2,
    IEEE802_1X_TYPE_EAPOL_KEY = 3,
    IEEE802_1X_TYPE_EAPOL_ENCAPSULATED_ASF_ALERT = 4
};
#endif  // ATH_KF

#define EAPOL_HDR_SIZE    4
#define LLC_SNAP_HDR_SIZE 8
#define MAX_TX_MTU        (ETHER_MAX_LEN - ETHER_HDR_LEN + 16)

#ifdef NT_FN_WPS
nt_status_t wps_recv_packet(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen);
void wps_parse_for_pbc(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen);
void wps_pbc_timer_handler(TimerHandle_t alarm, void *data);
void *wps_init(devh_t *dev);
void *wps_denit(devh_t *dev);
void wps_init_attrib(WPS_DEVICE_INFO *wpsInfo, NT_BOOL resetFlag);
void wps_cancel(devh_t *dev);
void wps_ap_init(devh_t *dev, conn_t *conn, conn_profile_t *cp, NT_BOOL bssConn);
void wps_ap_deinit(devh_t *dev, conn_t *conn, conn_profile_t *cp, NT_BOOL bssConn);
void wps_set_p2pDev(devh_t *dev, NT_BOOL set);
int wps_build_version(struct wpsbuf *msg);
int wps_build_req_type(struct wpsbuf *msg, WPS_REQUEST_TYPE type);
int wps_build_config_methods(struct wpsbuf *msg, uint16_t methods);
int wps_build_uuid_e(struct wpsbuf *msg, const uint8_t *uuid);
int wps_build_primary_dev_type(WPS_DEVICE_INFO *dev, struct wpsbuf *msg);
int wps_build_rf_bands(WPS_DEVICE_INFO *dev, struct wpsbuf *msg);
int wps_build_assoc_state(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_config_error(struct wpsbuf *msg, uint16_t err);
int wps_build_dev_password_id(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_dev_name(WPS_DEVICE_INFO *dev, struct wpsbuf *msg);
#endif  // NT_FN_WPS
/*****************************************************************************/
/* MACRO REDIRECTION DEFINITION START */
/*****************************************************************************/

/* these macros allow us to avoid spewing #ifdef's all over the code base */
//#ifdef NT_FN_WPS
#define WPS_INIT(dev)                       wps_init((dev))
#define WPS_RECV_PACKET(dev, eap_p)         wps_recv_packet((dev), (eap_p))
#define WPS_INIT_ATTRIB(devInfo, resetFlag) wps_init_attrib((devInfo), (resetFlag))
//#else /* NT_FN_WPS */
//#define WPS_INIT(dev)
//#define WPS_RECV_PACKET(dev, eap_p) A_ERROR /* returning A_ERROR should cause caller to proceed normally without wps
//*/ #define WPS_INIT_ATTRIB(devInfo, resetFlag) #endif /* NT_FN_WPS */

/*****************************************************************************/
/* MACRO REDIRECTION DEFINITION END */
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _WPS_API_H_ */
