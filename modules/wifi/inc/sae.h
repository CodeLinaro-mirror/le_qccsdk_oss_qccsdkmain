/*
 *
 * sae.h
 */

/*
 * Simultaneous authentication of equals
 * Copyright (c) 2012-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef CORE_WIFI_SECURITY_INC_SAE_H_
#define CORE_WIFI_SECURITY_INC_SAE_H_

#include <osapi.h>
#include "wlan_dev.h"

#ifdef NT_FN_WPA3
/* Special value returned by sae_parse_commit() */
#define SAE_SILENTLY_DISCARD 65535

#define IANA_SECP256R1 19

int get_sae_data(devh_t *dev, conn_t *conn, uint8_t *use_pt);
void sae_clear_data(struct sae_data *sae);
int sae_set_group(struct sae_data *sae, int group);
int sae_prepare_commit(const uint8_t *addr1, const uint8_t *addr2, const uint8_t *password, size_t password_len,
                       const char *identifier, struct sae_data *sae);
uint16_t sae_parse_commit(struct sae_data *sae, uint8_t *data, size_t bufLen,
                          const __attribute__((__unused__)) uint8_t **token,
                          size_t __attribute__((__unused__)) * token_len, int *allowed_groups);
int sae_process_commit(struct sae_data *sae);
int sae_check_confirm(struct sae_data *sae, uint8_t *data, size_t len);
int sae_cn_confirm_ecc(struct sae_data *sae, const uint8_t *sc, const struct crypto_bignum *scalar1,
                       const struct crypto_ec_point *element1, const struct crypto_bignum *scalar2,
                       const struct crypto_ec_point *element2, uint8_t *confirm);
int sae_cn_confirm_ffc(struct sae_data *sae, const uint8_t *sc, const struct crypto_bignum *scalar1,
                       const struct crypto_bignum *element1, const struct crypto_bignum *scalar2,
                       const struct crypto_bignum *element2, uint8_t *confirm);
struct sae_pt *sae_derive_pt(int *groups, const uint8_t *ssid, size_t ssid_len, const uint8_t *password,
                             size_t password_len, const char *identifier);
size_t sae_ecc_prime_len_2_hash_len(size_t prime_len);
void wpa_s_setup_sae_pt(devh_t *dev, conn_t *conn, ssid_t *ssid, struct sae_data *sae);
void sae_deinit_pt(struct sae_pt *pt);

#endif  // NT_FN_WPA3
#endif  /* CORE_WIFI_SECURITY_INC_SAE_H_ */
