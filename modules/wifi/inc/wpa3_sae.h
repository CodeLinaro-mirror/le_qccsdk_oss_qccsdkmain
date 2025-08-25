/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * wpa3_sae.h
 *
 *  Created on: Feb 22, 2022
 *
 */

#ifndef CORE_WIFI_SECURITY_INC_WPA3_SAE_H_
#define CORE_WIFI_SECURITY_INC_WPA3_SAE_H_

#include <stdint.h>
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef NT_FN_WPA3

#define SAE_KCK_LEN           32
#define SAE_PMK_LEN           32
#define SAE_PMKID_LEN         16
#define SAE_KEYSEED_KEY_LEN   32
#define SAE_MAX_PRIME_LEN     512
#define SAE_MAX_ECC_PRIME_LEN 66
#define SAE_COMMIT_MAX_LEN    (2 + 3 * SAE_MAX_PRIME_LEN)
#define SAE_CONFIRM_MAX_LEN   32

#define WLAN_RSNX_CAPAB_SAE_H2E 5

typedef struct sae_temporary_data {
    uint8_t kck[SAE_KCK_LEN];
    struct crypto_bignum *own_commit_scalar;
    struct crypto_bignum *own_commit_element_ffc;
    struct crypto_ec_point *own_commit_element_ecc;

    struct crypto_bignum *peer_commit_element_ffc;
    struct crypto_ec_point *peer_commit_element_ecc;

    struct crypto_ec_point *pwe_ecc;
    struct crypto_bignum *pwe_ffc;

    struct crypto_bignum *sae_rand;
    struct crypto_ec *ec;

    int prime_len;

    const struct dh_group *dh;

    const struct crypto_bignum *prime;
    const struct crypto_bignum *order;

    struct crypto_bignum *prime_buf;
    struct crypto_bignum *order_buf;

    char *pw_id;
    struct wpabuf *own_rejected_groups;
    unsigned int own_addr_higher : 1;
} sae_temporary_data;

enum {
    SAE_MSG_COMMIT = 1,
    SAE_MSG_CONFIRM = 2,
};

enum sae_state { SAE_NOTHING, SAE_COMMITTED, SAE_CONFIRMED, SAE_ACCEPTED };

typedef struct sae_data {
    enum sae_state state;
    uint16_t send_confirm;
    uint8_t pmk[SAE_PMK_LEN];
    uint8_t pmkid[SAE_PMKID_LEN];
    struct crypto_bignum *peer_commit_scalar;
    uint16_t group;
    unsigned int sync; /* protocol instance variable: Sync */
    uint16_t rc;       /* protocol instance variable: Rc (received send-confirm) */
    struct sae_temporary_data *tmp;
    uint8_t *g_sae_token;
    unsigned int h2e : 1;
} sae_data_t;

typedef struct sae_pt {
    struct sae_pt *next;
    int group;
    struct crypto_ec *ec;
    struct crypto_ec_point *ecc_pt;

    const struct dh_group *dh;
    struct crypto_bignum *ffc_pt;

#ifdef CONFIG_SAE_PK
    u8 ssid[32];
    size_t ssid_len;
#endif /* CONFIG_SAE_PK */
} sae_pt_t;

struct sae_pk {
    struct wpabuf *m;
    struct crypto_ec_key *key;
    int group;
    struct wpabuf *pubkey; /* DER encoded subjectPublicKey */

#ifdef CONFIG_TESTING_OPTIONS
    struct crypto_ec_key *sign_key_override;
#endif /* CONFIG_TESTING_OPTIONS */
};

#endif  // NT_FN_WPA3

#endif /* CORE_WIFI_SECURITY_INC_WPA3_SAE_H_ */
