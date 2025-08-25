/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _IF_EAPOL_H_
#define _IF_EAPOL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct eapol_header_s {
    uint8_t protoVers;
    uint8_t type;
    uint16_t bodyLength;
} __ATTRIB_PACK EAPOL_HEADER, *PEAPOL_HEADER;

#define EAPOL_PROTOVERS      0x01
#define EAPOL_TYPE_EAP       0x00
#define EAPOL_TYPE_START     0x01
#define EAPOL_TYPE_LOGOFF    0x02
#define EAPOL_TYPE_KEYMSG    0x03
#define EAPOL_TYPE_ASF_ALERT 0x04

#define EAPOL_KEY_TYPE_RC4  0x01
#define EAPOL_KEY_TYPE_WPA2 0x02
#define EAPOL_KEY_TYPE_WPA  0xFE

/*
 *  EAPOL-Key message, type 1-RC4
 */
typedef struct EAPOL_KEY_RC4 {
    uint8_t descType;
    uint16_t keyLength;
    uint16_t keyReplayCtr;
    uint8_t keyIV[16];
    uint8_t keyIndex;
#define KEY_USAGE_BROADCAST 0x00
#define KEY_USAGE_UNICAST   0x80
    uint8_t keySig[16];
} __ATTRIB_PACK EAPOL_KEY_RC4, *PEAPOL_KEY_RC4;

typedef struct EAPOL_KEY_INFO {
    union {
        struct {
#ifdef BIG_ENDIAN
            uint16_t reserved : 3,
                encrKeyData : 1,  // KeyData field is encrypted
                request : 1,      // STA wants a new key
                error : 1,        // STA saw a MIC error
                secure : 1,       // set if PW keys in use
                mic : 1,          // set iff msg is MICd
                ack : 1,          // STA sets this, AP clears it
                keyUsage : 1,     // if PW, this is install bit
                                  // if group, it's rx/both bit
                keyIndex : 2,     // 802.11 key index
                keyType : 1,      // 1 = PW, 0 = group
                type : 3;         // 1 for HMAC-MD5, 2 for AES-KEYWRAP
#else
            uint16_t type : 3, keyType : 1, keyIndex : 2, keyUsage : 1, ack : 1, mic : 1, secure : 1, error : 1,
                request : 1, encrKeyData : 1, reserved : 3;
#endif
        } k;
        uint16_t word;
    } u;
} __ATTRIB_PACK EAPOL_KEY_INFO;

#define EAPOL_KEY_INFO_TYPE_TKIP       1  // HMAC-MD5 and RC4
#define EAPOL_KEY_INFO_TYPE_AES        2  // HMAC_SHA1 and AES-KEYWRAP
#define WPA_KEY_INFO_TYPE_AES_128_CMAC 3  // AES_128_CMAC and AES-KEYWRAP
#define EAPOL_KEY_INFO_TYPE_MASK       0x0007

#define EAPOL_KEY_INFO_KEYTYPE_PAIRWISE 1
#define EAPOL_KEY_INFO_KEYTYPE_GROUP    0
#define EAPOL_KEY_INFO_KEYTYPE_MASK     0x0008
#define EAPOL_KEY_INFO_INSTALL_MASK     (1 << 6)
#define EAPOL_KEY_INFO_ACK_MASK         (1 << 7)
#define EAPOL_KEY_INFO_MIC_MASK         (1 << 8)
#define EAPOL_KEY_INFO_SECURE_MASK      (1 << 9)
#define EAPOL_KEY_INFO_WPA_INDEX_SHIFT  4
#define EAPOL_KEY_INFO_WPA_INDEX_MASK   (0x3 << EAPOL_KEY_INFO_WPA_INDEX_SHIFT)

#define GET_EAPOL_KEY_WPA_INDEX(info) (((info)&EAPOL_KEY_INFO_WPA_INDEX_MASK) >> EAPOL_KEY_INFO_WPA_INDEX_SHIFT)

#define IS_EAPOL_KEY_INFO_KEYTYPE_PAIRWISE(k) ((k)&EAPOL_KEY_INFO_KEYTYPE_MASK)

#define EAPOL_KEY_INFO_ERROR_MASK  0x0400
#define IS_EAPOL_KEY_INFO_ERROR(k) ((k)&EAPOL_KEY_INFO_ERROR_MASK)

#define EAPOL_KEY_MIC_LENGTH 16
#define EAPOL_KEY_RSC_LENGTH 8

typedef struct EAPOL_KEY_WPA {
    uint8_t descType;
    EAPOL_KEY_INFO keyInfo;
    uint16_t keyLength;
    uint8_t replayCtr[8];
    uint8_t keyNonce[32];
    uint8_t keyIV[16];
    uint8_t keyRSC[8];
    uint8_t keyID[8];  // Not used for WPAv2
    uint8_t keyMIC[16];
    uint16_t keyDataLength;
    uint8_t keyData[1];  // 0 or more octets
} __ATTRIB_PACK EAPOL_KEY_WPA;

#define EAPOL_ETHER_TYPE 0x888e

typedef struct eap_header_s {
    uint8_t code;       /* EAP code */
    uint8_t identifier; /* EAP specific ID for matching */
    uint16_t length;    /* includes header and body */
} __ATTRIB_PACK EAP_HEADER, *PEAP_HEADER;

typedef struct eap_type_header_s {
    uint8_t type; /* type code for req/response */
                  /* start of variable length data */
} __ATTRIB_PACK EAP_REQ_RESP_HEADER;

#define EAP_CODE_REQ_TYPE_IDENTITY 1 /* EAP request type Identity */

#endif  // _IF_EAPOL_H_
