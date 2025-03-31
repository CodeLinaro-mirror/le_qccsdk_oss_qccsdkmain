#include <stdio.h>
#include <assert.h>
#include "osapi.h"
#include "wifi_cmn.h"

#include "wps_def.h"
#include "wlan_dev.h"

void wps_parse_ie(__unused devh_t *dev, WPS_CONTEXT *wps, uint8_t *pBuffer, uint16_t length)
{
    return;
}
void * wps_init(devh_t *dev)
{
    return NULL;
}
void * wps_denit(devh_t *dev)
{
    return NULL;
}
void wmi_wps_start(devh_t *dev, WPS_CONTEXT *wps, WMI_WPS_START_CMD *pWpsStart)
{
    return;
}
void wmi_wps_set_config(devh_t *dev, WMI_WPS_START_CMD *buf, WPS_CONTEXT *wps)
{
    return;
}
void wps_start_connect_process(devh_t *dev, WPS_CONTEXT *wps, uint8_t auth_type,
    uint16_t encr_type, uint8_t pack_wps_ie,WPS_CREDENTIAL *wps_cred) 
{
    return;
}
void wps_association_complete_event(devh_t *dev)
{
    return;
}
nt_status_t wps_recv_packet(devh_t *dev, uint8_t *bufPtr, uint16_t  __attribute__((__unused__)) bufLen)
{
    nt_status_t status = NT_ERRMAX;
    return status;
}
void wps_ap_init (devh_t *dev, conn_t *conn,  conn_profile_t *cp, NT_BOOL bssConn)
{
    return;
}
void wps_ap_deinit(devh_t *dev, conn_t *conn, conn_profile_t *cp, NT_BOOL bssConn)
{
    return;
}