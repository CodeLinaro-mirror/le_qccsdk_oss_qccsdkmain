/*
 */

/*
 * EAP WSC
 * Copyright (c) 2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#ifndef EAP_WSC_H
#define EAP_WSC_H

#ifdef NT_FN_WPS
// struct wpsbuf *eap_msg_alloc(int vendor, EapType type, uint16_t payload_len, uint8_t code, uint8_t identifier,
//                              uint8_t wsc_pkt);
struct wpsbuf *eap_wsc_build_msg(WPS_CONTEXT *wps, uint8_t id, uint8_t *ignore);
int eap_wsc_process_cont(WPS_CONTEXT *wps, const uint8_t *buf, uint16_t len, uint8_t op_code);
struct wpsbuf *eap_wsc_build_frag_ack(uint8_t id, uint8_t code);
struct wpsbuf *eap_wsc_build_start(WPS_CONTEXT *wps, uint8_t id);
struct wpsbuf *eap_wsc_process_fragment(WPS_CONTEXT *wps, uint8_t id, uint8_t flags, uint8_t op_code,
                                        uint16_t message_length, const uint8_t *buf, uint16_t len, uint8_t *ignore);
struct wpsbuf *eap_wsc_process(WPS_CONTEXT *wps, uint8_t id, uint8_t *ignore, uint8_t *reqData, int16_t len);
struct wpsbuf *eap_build_enrollee_identity_resp(uint8_t id);
struct wpsbuf *eap_build_registrar_identity_req(uint8_t id);
struct wpsbuf *eap_build_registrar_failure_req(uint8_t id);

void eap_req_process(devh_t *dev, WPS_CONTEXT *wps, uint8_t *eap_pkt, uint8_t id, uint8_t eap_code, int16_t eap_len);
#endif  // NT_FN_WPS
void eap_req_thread_start(TimerHandle_t alarm, void *data);
void eap_req_thread_bgr_handler(void *arg, uint32_t unused_bgr_id);

#endif /* EAP_WSC_H */
