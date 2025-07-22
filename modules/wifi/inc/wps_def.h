/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WPS_DEF_H_
#define _WPS_DEF_H_
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_timer.h"
#include "wlan_dev.h"

#ifdef NT_FN_WPS

//#define WPS_DEBUG
#define WPS_PRINTF(...)
#define CONFIG_WPS2 1
/* Default Values -Start */
#define WPS_DEFAULT_MANUFACTURER   "Qualcomm"
#define WPS_DEFAULT_MODEL_NAME     "ModelName"
#define WPS_DEFAULT_MODEL_NO       "123456790"
#define WPS_DEFAULT_SERIAL_NO      "123456"
#define WPS_DEFAULT_DEV_NAME       "IOT-NT"
#define WPS_DEFAULT_UUID_R         "ABCDEFABCDABBAAB" /* 16 Bytes */
#define WPS_DEFAULT_UUID           "ABCDEFABCDEFABCD" /* 16 Bytes */
#define WPS_DEFAULT_CONFIG_METHODS WPS_CONFIG_LABEL | WPS_CONFIG_DISPLAY | WPS_CONFIG_PUSHBUTTON | WPS_CONFIG_KEYPAD

#define WPS_DEFAULT_DEV_CATEG    WPS_DEV_NETWORK_INFRA
#define WPS_DEFAULT_DEV_SUBCATEG WPS_DEV_COMPUTER_PC

#define WPS_DEFAULT_OS_VERSION 0x0
/* Default values - End */

#ifdef CONFIG_WPS2
#define WPS_VERSION 0x20
#else /* CONFIG_WPS2 */
#define WPS_VERSION 0x10
#endif /* CONFIG_WPS2 */

#define WPS_DEV_OUI_WFA  0x0050f204
#define WPS_DEV_TYPE_LEN 8

#define EAP_VENDOR_WFA      0x00372A /* Wi-Fi Alliance */
#define EAP_VENDOR_TYPE_WSC 1
#define WPS_VENDOR_ID_WFA   14122

#define WLAN_EID_VENDOR_SPECIFIC 221

#define WSC_FLAGS_MF 0x01
#define WSC_FLAGS_LF 0x02

#define FALSE 0
#define TRUE  1

#define WSC_ID_ENROLLEE     "WFA-SimpleConfig-Enrollee-1-0"
#define WSC_ID_ENROLLEE_LEN 29

#define WSC_ID_REGISTRAR     "WFA-SimpleConfig-Registrar-1-0"
#define WSC_ID_REGISTRAR_LEN 30

#define PSK_HEX_KEY_LEN 64

#define WPS_STOP_SCAN 0x1

#define SEC_TO_MSEC(x) (x * 1000)

/* WPS Walk Timer Min and Default Values */
#define WPS_TIMEOUT_MIN     120 /* in sec */
#define WPS_DEFAULT_TIMEOUT WPS_TIMEOUT_MIN

/* Individual Message Timeout Value */
#define WPS_MSG_TIMEOUT   (SEC_TO_MSEC(4))
#define WPS_MSG_MAX_RETRY 3

/* Connection Timeout Value */
#define WPS_CONNECTION_TIMEOUT (SEC_TO_MSEC(10))
#define WPS_CON_MAX_RETRY      5

#define WPS_SET_DEFAULT(src, val)                           \
    if (src)                                                \
        nt_osal_free_memory(src);                           \
    src = nt_osal_allocate_memory(strlen((char *)val) + 1); \
    if (src)                                                \
        memset(src, 0, strlen((char *)val) + 1);            \
    if (src)                                                \
        memscpy(src, strlen((char *)val), val, strlen((char *)val));

#define WPS_CHECK_FOR_RETRANSMISSION(ptx_state, cur_state, attr)          \
    if ((wps->prev_tx_state == ptx_state) && (wps->state == cur_state)) { \
        wps->flag |= WPS_RETRANSMIT_LAST_TX_PKT;                          \
        free(attr);                                                       \
        return WPS_CONTINUE;                                              \
    }

#define BEACON_ID 1
#define PROBE_ID  2

#define WPS_ROLE_NONE        0
#define WPS_AP_ENROLLEE_ROLE 3

struct eap_hdr {
    uint8_t code;
    uint8_t identifier;
    uint16_t length;
} __ATTRIB_PACK;

typedef enum {
    EAP_TYPE_IDENTITY = 1,   /* RFC 3748 */
    EAP_TYPE_EXPANDED = 254, /* RFC 3748 */
    EAP_TYPE_EAPOL_START = 0xFF,
} EapType;

enum { EAP_CODE_NONE = 0, EAP_CODE_REQUEST = 1, EAP_CODE_RESPONSE = 2, EAP_CODE_SUCCESS = 3, EAP_CODE_FAILURE = 4 };

struct eapol_hdr {
    uint8_t version;
    uint8_t type;
    uint16_t length;
} __ATTRIB_PACK;

/* Response Type */
typedef enum _WPS_RESPONSE_TYPE {
    WPS_RESP_ENROLLEE_INFO = 0,
    WPS_RESP_ENROLLEE = 1,
    WPS_RESP_REGISTRAR = 2,
    WPS_RESP_AP = 3
} WPS_RESPONSE_TYPE;

/* WFA Vendor Extension subelements */
enum {
    WFA_ELEM_VERSION2 = 0x00,
    WFA_ELEM_AUTHORIZEDMACS = 0x01,
    WFA_ELEM_NETWORK_KEY_SHAREABLE = 0x02,
    WFA_ELEM_REQUEST_TO_ENROLL = 0x03,
    WFA_ELEM_SETTINGS_DELAY_TIME = 0x04
};

struct wpsbuf {
    unsigned int size;       /* total size of the allocated buffer */
    unsigned int used;       /* length of data in the buffer */
    unsigned char *ext_data; /* pointer to external data; NULL if data follows
                              * struct wpsbuf */
    /* optionally followed by the allocated buffer */
};

#define ETH_ALEN             6
#define WPS_PSK_LEN          16
#define WPS_AUTHKEY_LEN      32
#define WPS_EMSK_LEN         32
#define WPS_KEYWRAPKEY_LEN   16
#define WPS_HASH_LEN         32
#define WPS_NONCE_LEN        16
#define WPS_UUID_LEN         16
#define WPS_MAX_UID_LEN      16
#define WPS_SECRET_NONCE_LEN 16
#define WPS_PIN_LENGTH       8

/* Device Info */
typedef struct _WPS_DEVICE_INFO {
    uint8_t *manufacturer;
    uint8_t *model_name;
    uint8_t *model_number;
    uint8_t *serial_number;
    uint8_t pri_dev_type[8];
    uint8_t *device_name;
    uint8_t uuid[WPS_MAX_UID_LEN];
    uint8_t mac_addr_e[ETH_ALEN];
    uint8_t mac_addr[ETH_ALEN];
    uint32_t os_version;
    uint8_t rf_bands;
    uint16_t config_methods;
    uint16_t dev_pw_id;
} WPS_DEVICE_INFO;

typedef struct _WPS_KEY_INFO {
    uint8_t nonce_e[WPS_NONCE_LEN];
    uint8_t nonce_r[WPS_NONCE_LEN];

    uint8_t snonce[2 * WPS_SECRET_NONCE_LEN];
    struct wpsbuf *dh_privkey;

    struct wpsbuf *dh_pubkey_e;
    struct wpsbuf *dh_pubkey_r;

    uint8_t psk1[WPS_PSK_LEN];
    uint8_t psk2[WPS_PSK_LEN];

    uint8_t authkey[WPS_AUTHKEY_LEN];

    uint8_t uuid_e[WPS_UUID_LEN];
    uint8_t uuid_r[WPS_UUID_LEN];

    uint8_t emsk[WPS_EMSK_LEN];
    uint8_t keywrapkey[WPS_KEYWRAPKEY_LEN];

    uint8_t peer_hash1[WPS_HASH_LEN];
    uint8_t peer_hash2[WPS_HASH_LEN];
    uint8_t role;
    uint8_t *pub_key;
} WPS_KEY_INFO;

/* Request Type */
typedef enum _WPS_REQUEST_TYPE {
    WPS_REQ_ENROLLEE_INFO = 0,
    WPS_REQ_ENROLLEE = 1,
    WPS_REQ_REGISTRAR = 2,
    WPS_REQ_WLAN_MANAGER_REGISTRAR = 3,
} WPS_REQUEST_TYPE;

/* Authentication Type Flags */
#define WPS_AUTH_OPEN        0x0001
#define WPS_AUTH_WPAPSK      0x0002
#define WPS_AUTH_SHARED      0x0004
#define WPS_AUTH_WPA         0x0008
#define WPS_AUTH_WPA2        0x0010
#define WPS_AUTH_WPA2PSK     0x0020
#define WPS_AUTH_WPA3PSK     0x0040
#define WPS_AUTH_WPA2WPA3PSK 0x0080
#define WPS_AUTH_TYPES       (WPS_AUTH_OPEN | WPS_AUTH_WPA2PSK)

/* Encryption Type Flags */
#define WPS_ENCR_NONE  0x0001
#define WPS_ENCR_WEP   0x0002
#define WPS_ENCR_TKIP  0x0004
#define WPS_ENCR_AES   0x0008
#define WPS_ENCR_TYPES (WPS_ENCR_NONE | WPS_ENCR_AES)

/* Configuration Error */
enum WPS_CONFIG_ERROR {
    WPS_CFG_NO_ERROR = 0,
    WPS_CFG_OOB_IFACE_READ_ERROR = 1,
    WPS_CFG_DECRYPTION_CRC_FAILURE = 2,
    WPS_CFG_24_CHAN_NOT_SUPPORTED = 3,
    WPS_CFG_50_CHAN_NOT_SUPPORTED = 4,
    WPS_CFG_SIGNAL_TOO_WEAK = 5,
    WPS_CFG_NETWORK_AUTH_FAILURE = 6,
    WPS_CFG_NETWORK_ASSOC_FAILURE = 7,
    WPS_CFG_NO_DHCP_RESPONSE = 8,
    WPS_CFG_FAILED_DHCP_CONFIG = 9,
    WPS_CFG_IP_ADDR_CONFLICT = 10,
    WPS_CFG_NO_CONN_TO_REGISTRAR = 11,
    WPS_CFG_MULTIPLE_PBC_DETECTED = 12,
    WPS_CFG_ROGUE_SUSPECTED = 13,
    WPS_CFG_DEVICE_BUSY = 14,
    WPS_CFG_SETUP_LOCKED = 15,
    WPS_CFG_MSG_TIMEOUT = 16,
    WPS_CFG_REG_SESS_TIMEOUT = 17,
    WPS_CFG_DEV_PASSWORD_AUTH_FAILURE = 18
};

/* Vendor specific Error Indication for WPS event messages */
enum wps_error_indication {
    WPS_EI_NO_ERROR,
    WPS_EI_SECURITY_TKIP_ONLY_PROHIBITED,
    WPS_EI_SECURITY_WEP_PROHIBITED,
    NUM_WPS_EI_VALUES
};

/* RF Bands */
#define WPS_RF_24GHZ 0x01
#define WPS_RF_50GHZ 0x02

/* Config Methods */
#define WPS_CONFIG_USBA          0x0001
#define WPS_CONFIG_ETHERNET      0x0002
#define WPS_CONFIG_LABEL         0x0004
#define WPS_CONFIG_DISPLAY       0x0008
#define WPS_CONFIG_EXT_NFC_TOKEN 0x0010
#define WPS_CONFIG_INT_NFC_TOKEN 0x0020
#define WPS_CONFIG_NFC_INTERFACE 0x0040
#define WPS_CONFIG_PUSHBUTTON    0x0080
#define WPS_CONFIG_KEYPAD        0x0100
#ifdef CONFIG_WPS2
#define WPS_CONFIG_VIRT_PUSHBUTTON 0x0280
#define WPS_CONFIG_PHY_PUSHBUTTON  0x0480
#define WPS_CONFIG_VIRT_DISPLAY    0x2008
#define WPS_CONFIG_PHY_DISPLAY     0x4008
#endif /* CONFIG_WPS2 */
#define WPS_CONFIG_MASK 0xFFFF
/* Connection Type Flags */
#define WPS_CONN_ESS  0x01
#define WPS_CONN_IBSS 0x02

/* Wi-Fi Protected Setup State */
enum WPS_STATE { WPS_STATE_NOT_CONFIGURED = 1, WPS_STATE_CONFIGURED = 2 };

/* Association State */
enum WPS_ASSOC_STATE {
    WPS_ASSOC_NOT_ASSOC = 0,
    WPS_ASSOC_CONN_SUCCESS = 1,
    WPS_ASSOC_CFG_FAILURE = 2,
    WPS_ASSOC_FAILURE = 3,
    WPS_ASSOC_IP_FAILURE = 4
};

/* Device Password ID */
enum WPS_DEV_PASSWORD_ID {
    DEV_PW_DEFAULT = 0x0000,
    DEV_PW_USER_SPECIFIED = 0x0001,
    DEV_PW_MACHINE_SPECIFIED = 0x0002,
    DEV_PW_REKEY = 0x0003,
    DEV_PW_PUSHBUTTON = 0x0004,
    DEV_PW_REGISTRAR_SPECIFIED = 0x0005
};

/* Message Type */
typedef enum _WPS_MSG_TYPE {
    WPS_Beacon = 0x01,
    WPS_ProbeRequest = 0x02,
    WPS_ProbeResponse = 0x03,
    WPS_M1 = 0x04,
    WPS_M2 = 0x05,
    WPS_M2D = 0x06,
    WPS_M3 = 0x07,
    WPS_M4 = 0x08,
    WPS_M5 = 0x09,
    WPS_M6 = 0x0a,
    WPS_M7 = 0x0b,
    WPS_M8 = 0x0c,
    WPS_WSC_ACK = 0x0d,
    WPS_WSC_NACK = 0x0e,
    WPS_WSC_DONE = 0x0f
} WPS_MSG_TYPE;

typedef enum _WSC_OP_CODE {
    WSC_UPnP = 0 /* No OP Code in UPnP transport */,
    WSC_Start = 0x01,
    WSC_ACK = 0x02,
    WSC_NACK = 0x03,
    WSC_MSG = 0x04,
    WSC_Done = 0x05,
    WSC_FRAG_ACK = 0x06
} WSC_OP_CODE;

/*enum wps_process_res - WPS message processing result */
enum wps_process_res { WPS_DONE, WPS_CONTINUE, WPS_FAILURE, WPS_PENDING };

#define MAX_SSID_LEN       32
#define ETH_ALEN           6
#define AP_DEFAULT_CHANNEL 2412

/* WPS Credential */
typedef struct _WPS_CREDENTIALS {
    uint8_t ssid[MAX_SSID_LEN + 1];
    uint8_t ssid_len;
    uint8_t key_idx;
    uint8_t key[64];
    uint8_t key_len;
    uint8_t mac_addr[ETH_ALEN];
    uint16_t auth_type;
    uint16_t encr_type;
    uint8_t cred_flag;
} WPS_CREDENTIALS;

typedef enum WPS_DEVICE_CATEG {
    WPS_DEV_COMPUTER = 1,
    WPS_DEV_INPUT = 2,
    WPS_DEV_PRINTER = 3,
    WPS_DEV_CAMERA = 4,
    WPS_DEV_STORAGE = 5,
    WPS_DEV_NETWORK_INFRA = 6,
    WPS_DEV_DISPLAY = 7,
    WPS_DEV_MULTIMEDIA = 8,
    WPS_DEV_GAMING = 9,
    WPS_DEV_PHONE = 10,
} WPS_DEVICE_CATEG;

typedef enum WPS_DEV_SUBCATEG {
    WPS_DEV_COMPUTER_PC = 1,
    WPS_DEV_COMPUTER_SERVER = 2,
    WPS_DEV_COMPUTER_MEDIA_CENTER = 3,
    WPS_DEV_COMPUTER_MID = 7,
    WPS_DEV_PRINTER_PRINTER = 1,
    WPS_DEV_PRINTER_SCANNER = 2,
    WPS_DEV_CAMERA_DIGITAL_STILL_CAMERA = 1,
    WPS_DEV_STORAGE_NAS = 1,
    WPS_DEV_NETWORK_INFRA_AP = 1,
    WPS_DEV_NETWORK_INFRA_ROUTER = 2,
    WPS_DEV_NETWORK_INFRA_SWITCH = 3,
    WPS_DEV_DISPLAY_TV = 1,
    WPS_DEV_DISPLAY_PICTURE_FRAME = 2,
    WPS_DEV_DISPLAY_PROJECTOR = 3,
    WPS_DEV_MULTIMEDIA_DAR = 1,
    WPS_DEV_MULTIMEDIA_PVR = 2,
    WPS_DEV_MULTIMEDIA_MCX = 3,
    WPS_DEV_GAMING_XBOX = 1,
    WPS_DEV_GAMING_XBOX360 = 2,
    WPS_DEV_GAMING_PLAYSTATION = 3,
    WPS_DEV_PHONE_WINDOWS_MOBILE = 1
} WPS_DEV_SUBCATEG;

#define WPS_MAX_UID_LEN    16
#define WPS_MAX_PASSWD_LEN 10
#define WPS_KWA_LEN        8
#define SSID_MAX_LEN       32
#define WPS_DEV_TYPE_LEN   8

#define DH_PRI_A_KEY_LEN 192
#define DH_PUB_KEY_LEN   192

#define WPS_AUTHENTICATOR_LEN 8
#define G5_PRIME_LEN          192
#define G5_BASE_LEN           1

#define WPS_OOB_DEVICE_PASSWORD_ATTR_LEN 54

typedef struct _WPS_PARSE_ATTR {
    const uint8_t *version;                /* 1 octet */
    const uint8_t *msg_type;               /* 1 octet */
    const uint8_t *enrollee_nonce;         /* WPS_NONCE_LEN (16) octets */
    const uint8_t *registrar_nonce;        /* WPS_NONCE_LEN (16) octets */
    const uint8_t *uuid_r;                 /* WPS_UUID_LEN (16) octets */
    const uint8_t *uuid_e;                 /* WPS_UUID_LEN (16) octets */
    const uint8_t *auth_type_flags;        /* 2 octets */
    const uint8_t *encr_type_flags;        /* 2 octets */
    const uint8_t *conn_type_flags;        /* 1 octet */
    const uint8_t *config_methods;         /* 2 octets */
    const uint8_t *sel_reg_config_methods; /* 2 octets */
    const uint8_t *primary_dev_type;       /* 8 octets */
    const uint8_t *rf_bands;               /* 1 octet */
    const uint8_t *assoc_state;            /* 2 octets */
    const uint8_t *config_error;           /* 2 octets */
    const uint8_t *dev_password_id;        /* 2 octets */
    const uint8_t *oob_dev_password;       /* WPS_OOB_DEVICE_PASSWORD_ATTR_LEN (54)
                                            * octets */
    const uint8_t *os_version;             /* 4 octets */
    const uint8_t *wps_state;              /* 1 octet */
    const uint8_t *authenticator;          /* WPS_AUTHENTICATOR_LEN (8) octets */
    const uint8_t *r_hash1;                /* WPS_HASH_LEN (32) octets */
    const uint8_t *r_hash2;                /* WPS_HASH_LEN (32) octets */
    const uint8_t *e_hash1;                /* WPS_HASH_LEN (32) octets */
    const uint8_t *e_hash2;                /* WPS_HASH_LEN (32) octets */
    const uint8_t *r_snonce1;              /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *r_snonce2;              /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *e_snonce1;              /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *e_snonce2;              /* WPS_SECRET_NONCE_LEN (16) octets */
    const uint8_t *key_wrap_auth;          /* WPS_KWA_LEN (8) octets */
    const uint8_t *auth_type;              /* 2 octets */
    const uint8_t *encr_type;              /* 2 octets */
    const uint8_t *network_idx;            /* 1 octet */
    const uint8_t *network_key_idx;        /* 1 octet */
    const uint8_t *mac_addr;               /* ETH_ALEN (6) octets */
    const uint8_t *key_prov_auto;          /* 1 octet (Bool) */
    const uint8_t *dot1x_enabled;          /* 1 octet (Bool) */
    const uint8_t *selected_registrar;     /* 1 octet (Bool) */
    const uint8_t *request_type;           /* 1 octet */
    const uint8_t *response_type;          /* 1 octet */
    const uint8_t *ap_setup_locked;        /* 1 octet */

    /* variable length fields */
    const uint8_t *manufacturer;
    const uint8_t *model_name;
    const uint8_t *model_number;
    const uint8_t *serial_number;
    const uint8_t *dev_name;
    const uint8_t *public_key;
    const uint8_t *encr_settings;
    const uint8_t *ssid;         /* <= 32 octets */
    const uint8_t *network_key;  /* <= 64 octets */
    const uint8_t *eap_type;     /* <= 8 octets */
    const uint8_t *eap_identity; /* <= 64 octets */

    uint16_t manufacturer_len;
    uint16_t model_name_len;
    uint16_t model_number_len;
    uint16_t serial_number_len;
    uint16_t dev_name_len;
    uint16_t public_key_len;
    uint16_t encr_settings_len;
    uint16_t ssid_len;
    uint16_t network_key_len;
    uint16_t eap_type_len;
    uint16_t eap_identity_len;

    /* attributes that can occur multiple times */
#define MAX_CRED_COUNT 10
    uint16_t num_cred;
    const uint8_t *cred[MAX_CRED_COUNT];
    uint16_t cred_len[MAX_CRED_COUNT];
} WPS_PARSE_ATTR;

typedef enum { PIN_MODE = 0x1, PBC_MODE = 0x2 } WPS_OP_MODE;

#define WPS_BAD_AP_MAX 4

typedef struct _WPS_START_INFO_FROM_HOST {
    uint8_t ssid[MAX_SSID_LEN + 1];
    uint8_t ssid_len;
    uint8_t peer_mac_addr[ETH_ALEN];
    uint16_t channel;
    WPS_OP_MODE config_mode;
    uint8_t dev_password[WPS_MAX_PASSWD_LEN];
    uint16_t dev_password_len;
    uint8_t timeout;
    uint8_t bad_ap_mac[WPS_BAD_AP_MAX][ETH_ALEN];
    uint8_t bad_ap_count;
} WPS_START_INFO_FROM_HOST;

typedef struct WPS_INPUT_INFO {
    uint8_t ssid[MAX_SSID_LEN + 1];
    uint8_t ssid_len;
    uint8_t peer_mac_addr[ETH_ALEN];
    uint16_t channel;
} WPS_INPUT_INFO;

#define GET_DEV_WPS_CTX(dev, wps) (wps = (WPS_CONTEXT *)(dev)->wps_ctx)
#define PUT_DEV_WPS_CTX(dev, wps) ((dev)->wps_ctx = (void *)(wps))

#define IS_WPS_OFFLOAD_INITIALIZED(dev) (((dev)->wps_ctx == NULL) ? FALSE : TRUE)

#define PUT_DEV_CTX(dev, wps) ((wps)->parent_dev = (dev))
#define GET_DEV_CTX(dev, wps) (dev = (wps)->parent_dev)

#define WPS_W4_FOR_CONNECT_EVENT         0x0001
#define WPS_IN_PROGRESS                  0x0002
#define WPS_DROP_CON_DIS_EVENTS          0x0004
#define ADD_WPS_SUCCESS_STS_IN_CON_EVENT 0x0008
#define WPS_SCAN_IN_PROGRESS             0x0010
#define WPS_REGISTRAR_FOUND              0x0020
#define WPS_PBC_OVERLAP                  0x0040
#define WPS_AUTO_SCAN                    0x0080
#define WPS_PIN_METHOD_IN_PROGRESS       0x0100
#define WPS_DO_AUTO_CONNECT              0x0200
#define WPS_SEND_PWD_ATH_FAIL_EVENT      0x0400
#define WPS_W4_RESP_PKT                  0x0800
#define WPS_RETRANSMIT_LAST_TX_PKT       0x1000
#define WPS_M2D_RECVD                    0x2000
#define WPS_HOST_INPUT_RECVD             0x4000
#define WPS_P2P_SUPPORT                  0x8000

#define MAX_PASSPHRASE_LEN         8
#define IOE_SSID_NAME              "IoE-SoftAP"
#define IOE_SSID_LENGTH            10
#define AP_PIN_FAILURE_RETRY_VAL   3
#define AP_PIN_FAILURE_TIMEOUT_MIN 1

struct eap_state {
    devh_t *dev;
    uint8_t eap_pkt[750];
    uint8_t id;
    uint8_t eap_code;
    uint16_t eap_len;
#ifdef AR6002_REV74
    A_TIMER eap_timer;
#else
    uint32_t wps_bgr_id;
#endif
};

typedef struct _WPS_CONTEXT {
    uint8_t config_error;
    enum {
        /* Enrollee states */
        SEND_EAPOL_START,
        SEND_INDENTITY_RESP,
        SEND_M1,
        RECV_M2,
        SEND_M3,
        RECV_M4,
        SEND_M5,
        RECV_M6,
        SEND_M7,
        RECV_M8,
        RECEIVED_M2D,
        WPS_MSG_DONE,
        RECV_ACK,
        WPS_FINISHED,
        SEND_WSC_NACK,
        RECV_EAPOL_START,
        SEND_IDENTITY_REQ,
        RECV_IDENTITY_RESP,
        RECV_M1,
        SEND_M2,
        RECV_M3,
        SEND_M4,
        RECV_M5,
        SEND_M6,
        RECV_M7,
        SEND_M8,
        SEND_M2D,
        SEND_WSC_START,
        RECV_M2D_ACK,
        RECV_DONE,
    } state,
        prev_tx_state;

    /* eap wsc state */
    enum { WAIT_START, MESG, FRAG_ACK, WAIT_FRAG_ACK, DONE, FAIL } eap_wsc_state;

    WPS_CREDENTIALS cred;
    WPS_DEVICE_INFO dev;
    WPS_DEVICE_INFO peer_dev;
    WPS_KEY_INFO key;

    uint16_t out_used;
    uint16_t fragment_size;

    struct wpsbuf *out_buf;
    struct wpsbuf *in_buf;
    struct wpsbuf *last_msg;

    WSC_OP_CODE out_op_code;
    WSC_OP_CODE in_op_code;

    WPS_START_INFO_FROM_HOST start_info;
    WPS_INPUT_INFO input_info;
    uint32_t flag;
    TimerHandle_t timer;

    TimerHandle_t retry_timer;
    uint8_t retry_count;
    struct wpsbuf *last_tx_msg;
    uint8_t last_txpkt_eapol_type;

    /* Device pointer where this structure linked */
    devh_t *parent_dev;
    uint16_t prev_act_dwell;  // placeholder for previosly set dwell time //Remove

    int softap_setup_locked;
    int softap_pin_failures;
    int softap_state;
    int force_pbc_overlap;
    int wps_pin_revealed;

    uint8_t role;
    uint8_t use_psk_key;
    uint8_t reg_done;
    uint16_t encr_types;
    uint8_t auth_type;
    uint8_t configured_state;
    void *active_ap_conn;
    NT_BOOL isDualBand;
    WPS_PARSE_ATTR *wps_attr;

    struct wpsbuf *dh_privkey_r;
    uint8_t *pub_key_r;
    uint8_t peer_sa[ETH_ALEN];
    uint8_t peer_uuid[WPS_MAX_UID_LEN];
    TimerHandle_t pbc_timer;
    uint8_t uuid[WPS_MAX_UID_LEN];
    uint8_t mac_addr_r[ETH_ALEN];
    TimerHandle_t deauth_timer;
    struct eap_state Eap_state;
    TimerHandle_t wps_pin_lock_timeout;
    uint8_t wps_init_flag;
    uint8_t auth_floor;
} WPS_CONTEXT;

#endif  // NT_FN_WPS

#endif /* _WPS_DEF_H_ */
