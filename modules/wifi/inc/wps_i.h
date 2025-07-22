/*
 */

/*
 */

/*
 * Wi-Fi Protected Setup - internal definitions
 * Copyright (c) 2008-2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef WPS_I_H
#define WPS_I_H

#ifdef NT_FN_WPS
/*---------- wps_common.c --------- */

void wps_kdf(const uint8_t *key, const uint8_t *label_prefix, size_t label_prefix_len, const char *label, uint8_t *res,
             size_t res_len);
int wps_derive_keys(WPS_CONTEXT *wps);
void wps_derive_psk(WPS_KEY_INFO *key, const uint8_t *dev_passwd, uint16_t dev_passwd_len);
struct wpsbuf *wps_decrypt_encr_settings(WPS_CONTEXT *wps, const uint8_t *encr, size_t encr_len);
void wps_send_m2d_recvd_info_to_host(WPS_CONTEXT *wps);
void wps_pwd_auth_fail_event(WPS_CONTEXT *wps, uint8_t part);

/* ------ wps_attr_build.c -------- */

struct wpsbuf *wps_ie_encapsulate(struct wpsbuf *data);
int wps_build_public_key(WPS_KEY_INFO *key, struct wpsbuf *msg, WPS_CONTEXT *wps);
int wps_build_resp_type(struct wpsbuf *msg, WPS_RESPONSE_TYPE type);
int wps_build_authenticator(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_msg_type(struct wpsbuf *msg, WPS_MSG_TYPE msg_type);
int wps_build_wfa_ext(struct wpsbuf *msg, int req_to_enroll, const uint8_t *auth_macs, size_t auth_macs_count);
int wps_build_enrollee_nonce(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_registrar_nonce(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_auth_type_flags(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_encr_type_flags(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_conn_type_flags(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_key_wrap_auth(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_encr_settings(WPS_KEY_INFO *key, struct wpsbuf *msg, struct wpsbuf *plain);

int wps_build_selected_registrar(WPS_CONTEXT *wps, struct wpsbuf *msg, uint8_t id);
int wps_build_ap_setup_locked(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_sel_reg_config_methods(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_mac_addr(WPS_DEVICE_INFO *dev, struct wpsbuf *msg);
int wps_build_wps_state(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_e_hash(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_e_snonce1(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_e_snonce2(WPS_KEY_INFO *key, struct wpsbuf *msg);
struct wpsbuf *wps_build_probe_req_ie(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_assoc_req_ie(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_probe_resp_ie(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_beacon_ie(WPS_CONTEXT *wps);
int wps_build_r_snonce1(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_r_snonce2(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_r_hash(WPS_KEY_INFO *key, struct wpsbuf *msg);
int wps_build_uuid_r(struct wpsbuf *msg, const uint8_t *uuid);
int wps_build_credential(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_ap_settings(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_cred_network_idx(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred_ssid(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred_auth_type(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred_encr_type(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred_network_key(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred_mac_addr(struct wpsbuf *msg, WPS_CREDENTIALS *cred);
int wps_build_cred(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_sel_pbc_reg_uuid_e(WPS_CONTEXT *wps, struct wpsbuf *msg);
int wps_build_registrar_nonce_zero(WPS_KEY_INFO *key, struct wpsbuf *msg);

/*---------- wps_attr_parse.c -----------*/

int wps_set_attr(WPS_PARSE_ATTR *attr, uint16_t type, const uint8_t *pos, uint16_t len);
int wps_parse_msg(const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);

/*------- wps_attr_process.c------ */

int wps_process_authenticator(WPS_CONTEXT *wps, const uint8_t *authenticator, const struct wpsbuf *msg);
int wps_process_key_wrap_auth(WPS_CONTEXT *wps, struct wpsbuf *msg, const uint8_t *key_wrap_auth);
int wps_process_cred_network_idx(WPS_CREDENTIALS *cred, const uint8_t *idx);
int wps_process_cred_ssid(WPS_CREDENTIALS *cred, const uint8_t *ssid, uint16_t ssid_len);
int wps_process_cred_auth_type(WPS_CREDENTIALS *cred, const uint8_t *auth_type);
int wps_process_cred_encr_type(WPS_CREDENTIALS *cred, const uint8_t *encr_type);
int wps_process_cred_network_key_idx(WPS_CREDENTIALS *cred, const uint8_t *key_idx);
int wps_process_cred_network_key(WPS_CREDENTIALS *cred, const uint8_t *key, uint16_t key_len);
int wps_process_cred_mac_addr(WPS_CREDENTIALS *cred, const uint8_t *mac_addr);
int wps_process_cred_eap_type(WPS_CREDENTIALS *cred, const uint8_t *eap_type, uint16_t eap_type_len);
int wps_process_cred_eap_identity(WPS_CREDENTIALS *cred, const uint8_t *identity, uint16_t identity_len);
int wps_process_cred_key_prov_auto(WPS_CREDENTIALS *cred, const uint8_t *key_prov_auto);
int wps_process_cred_802_1x_enabled(WPS_CREDENTIALS *cred, const uint8_t *dot1x_enabled);
void wps_workaround_cred_key(WPS_CREDENTIALS *cred);
int wps_process_cred(WPS_PARSE_ATTR *attr, WPS_CREDENTIALS *cred);

int wps_process_registrar_nonce(WPS_CONTEXT *wps, const uint8_t *r_nonce);
int wps_process_enrollee_nonce(WPS_CONTEXT *wps, const uint8_t *e_nonce);
int wps_process_e_hash1(WPS_CONTEXT *wps, const uint8_t *e_hash1);
int wps_process_e_hash2(WPS_CONTEXT *wps, const uint8_t *e_hash2);

int wps_process_uuid_r(WPS_CONTEXT *wps, const uint8_t *uuid_r);
int wps_process_uuid_e(WPS_CONTEXT *wps, const uint8_t *uuid_e);

int wps_process_mac_addr(WPS_CONTEXT *wps, const uint8_t *mac_addr);
int wps_process_auth_type_flags(WPS_CONTEXT *wps, const uint8_t *auth);
int wps_process_encr_type_flags(WPS_CONTEXT *wps, const uint8_t *encr);
int wps_process_conn_type_flags(WPS_CONTEXT *wps, const uint8_t *conn);
int wps_process_config_methods(WPS_CONTEXT *wps, const uint8_t *methods);
int wps_process_rf_bands(WPS_DEVICE_INFO *dev, const uint8_t *bands);
int wps_process_assoc_state(WPS_CONTEXT *wps, const uint8_t *assoc);
int wps_process_config_error(WPS_CONTEXT *wps, const uint8_t *err);
int wps_process_dev_password_id(WPS_CONTEXT *wps, const uint8_t *pw_id);
int wps_process_wps_state(WPS_CONTEXT *wps, const uint8_t *state);
int wps_process_pubkey(WPS_CONTEXT *wps, const uint8_t *pk, size_t pk_len);
int wps_process_r_hash1(WPS_CONTEXT *wps, const uint8_t *r_hash1);
int wps_process_r_hash2(WPS_CONTEXT *wps, const uint8_t *r_hash2);
int wps_process_r_snonce1(WPS_CONTEXT *wps, const uint8_t *r_snonce1);

int wps_process_e_snonce1(WPS_CONTEXT *wps, const uint8_t *e_snonce1);
int wps_process_r_snonce2(WPS_CONTEXT *wps, const uint8_t *r_snonce2);

int wps_process_e_snonce2(WPS_CONTEXT *wps, const uint8_t *e_snonce2);

int wps_process_cred_e(WPS_CONTEXT *wps, const uint8_t *cred, uint16_t cred_len);
int wps_process_creds(WPS_CONTEXT *wps, const uint8_t *cred[], uint16_t cred_len[], uint16_t num_cred);

/*------------- wps_registrar.c ----------------------- */

struct wpsbuf *wps_registrar_get_msg(WPS_CONTEXT *wps, WSC_OP_CODE *op_code);
enum wps_process_res wps_process_m1(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m3(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m5(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m7(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_wsc_done(WPS_CONTEXT *wps, const struct wpsbuf *msg);
enum wps_process_res wps_reg_process_wsc_msg(WPS_CONTEXT *wps, const struct wpsbuf *msg);
enum wps_process_res wps_registrar_process_msg(WPS_CONTEXT *wps, WSC_OP_CODE op_code, const struct wpsbuf *msg);
struct wpsbuf *wps_build_m2(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m2d(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m4(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m6(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m8(WPS_CONTEXT *wps);

/*------------- wps_enrollee.c ----------------------- */

struct wpsbuf *wps_build_m1(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m3(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m5(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_m7(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_wsc_done(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_wsc_ack(WPS_CONTEXT *wps);
struct wpsbuf *wps_build_wsc_nack(WPS_CONTEXT *wps);
struct wpsbuf *wps_enrollee_get_msg(WPS_CONTEXT *wps, WSC_OP_CODE *op_code);

enum wps_process_res wps_process_m2(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m2d(WPS_CONTEXT *wps, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m4(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m6(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_m8(WPS_CONTEXT *wps, const struct wpsbuf *msg, WPS_PARSE_ATTR *attr);
enum wps_process_res wps_process_wsc_msg(WPS_CONTEXT *wps, const struct wpsbuf *msg);
enum wps_process_res wps_process_wsc_ack(WPS_CONTEXT *wps, const struct wpsbuf *msg);
enum wps_process_res wps_process_wsc_nack(WPS_CONTEXT *wps, const struct wpsbuf *msg);
enum wps_process_res wps_enrollee_process_msg(WPS_CONTEXT *wps, WSC_OP_CODE op_code, const struct wpsbuf *msg);
#endif  // NT_FN_WPS
#endif  /* WPS_I_H */
