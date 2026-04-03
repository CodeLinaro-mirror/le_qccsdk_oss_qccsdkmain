/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_8021X_CFG_H_
#define _WLAN_8021X_CFG_H_

#define EAP_MD5
#define EAP_TLS
#define EAP_MSCHAPv2
#define EAP_PEAP
#define EAP_TTLS
#define CONFIG_SHA256
// #define CONFIG_INTERNAL_SHA384
// #define CONFIG_INTERNAL_SHA512
#define CONFIG_TLSV12
#define CONFIG_TLS_SKIP_SERVER_CERT_VERIFY
#define CONFIG_TLS_DISABLE_OCSP

#define IEEE8021X_EAPOL

#define CONFIG_NO_CONFIG_BLOBS
#define CONFIG_NO_RC4
#define CONFIG_NO_PBKDF2

#define CFG_EAPOL_PACKET_BUFFER_SIZE 1500

#define DEFAULT_EAPOL_FRAGMENT_SIZE 1398
#define DEFAULT_EAPOL_VERSION 1
#define DEFAULT_FASR_REAUTH 1
#define DEFAULT_EAP_WORKAROUND ((unsigned int)-1)

#define CONFIG_INPUT_METHOD_CNT (1)

/* pmk cache: pmk is cached in pmkid of eapol key packet */
#define CONFIG_ENABLE_PMK_CACHE

/* reauth: restart new seesion auth to update pmk, after old seesion auth is
 * completed */
#define CONFIG_ENABLE_REAUTH

#define CONFIG_TLS_INTERNAL_CLIENT
#define CONFIG_INTERNAL_LIBTOMMATH

#define CONFIG_ANSI_C_EXTRA

#define MBEDTLS_DEBUG_C

#endif /* _WLAN_8021X_CFG_H_ */
