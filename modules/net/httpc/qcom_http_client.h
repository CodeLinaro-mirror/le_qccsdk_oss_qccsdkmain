/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_ 1

#include "ip_addr.h"
#include "FreeRTOSConfig.h"

#include "mbedtls/ssl.h"
//#include "mbedtls/ssl_internal.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"
//#include "mbedtls/certs.h"
#include "mbedtls/platform.h"
#include "ssl_tls_alt.h"

#include "qapi_httpc.h"

/* Tuneable parameters
 *
 * It may be useful for for the porting engineer to modify one or more
 * parameters in this section.  However, normally this will not be
 * necessary.  Please contact InterNiche support if you have any questions.
 */
#define HTTP_DEFAULT_PORT  80
#define HTTPS_DEFAULT_PORT 443

#define HTTPC_SELECT_INTERVAL_MS       10
#define TICKS_PER_SEC                  configTICK_RATE_HZ
#define HTTPC_TIMER_TIMEOUT            (10 * TICKS_PER_SEC / 10) /*500ms*/
#define HTTPCTICKS                     sys_now()                 /* ticks since start */
#define HTTPCLIENT_DEFAULT_CON_SUPPORT 2
#define HTTPC_INVALID_HANDLE           (-1)

#define HTTPCLIENT_MAX_BUFFER_SIZE              1750
#define HTTPCLIENT_MIN_BUFFER_SIZE              512
#define HTTPCLIENT_MAX_URL_LENGTH               256  // was 256
#define HTTPCLIENT_MAX_HOST_LENGTH              64
#define HTTPCLIENT_DEFAULT_REMAIN_BUFFER_LENGTH 20  // was 1024

/* HTTP CLIENT flags */
#define HCF_CLOSED_BY_PEER     0x0001
#define HCF_RECV_HEADER        0x0002 /* all headers are received in rxbuffer */
#define HCF_CHUNKED            0x0004
#define HCF_CB_ADD_HEAD        0x0008
#define HCF_RESP_CONT          0x0010 /* already processed partial message body in last read */
#define HCF_TUNNEL_ESTABLISHED 0x0020 /* got an successful (i.e. 2xx) HTTP CONNECT response */
#define HCF_TUNNEL_TO_HTTPS    0x0040 /* origin server is an HTTPS server for HTTP CONNECT */
#define HCF_TLS_CACHED         0x0080 /* the server's tls fingerprint has been calculated and cached */

#define HTTPC_CHUNKED_MASK     0x80 /*HTTP chunk encoded*/
#define HTTPC_WITH_HEADER_MASK 0x01

/* HTTP PUBLIC KEY PIN EXTENTION*/
#define PINNED_HOST_MAX_NUM  4
#define FINGERPRINT_LENGTH   44
#define MAX_PING_NUM         4
#define MAX_CERTCHAIN_LENGTH 4
#define MAX_PKP_LIFETIME     5184000 /*seconds, equals to 60 days */

#define DEFAULT_SERVER_NAME "localhost"

#define TIME_EXP(e, n) ((e) <= (n))
#define TIME_ADD(n, d) ((n) + (d))

#define INVALID_SOCKET -1

#ifndef max
#define max(a, b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef min
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#endif

#define DEFAULT_IP_PREFER           0  // IP_V4
#define DEFAULT_PRE_ALLOCATE_BUFFER 0  // 0: don't PRE ALLOCATE, 1:PRE ALLOCATE

typedef enum { IP_V4, IP_V6 } ip_type;

typedef enum { CACHE_INVALID, CACHE_VALID, CACHE_TEMPCACHE } FP_CACHE_STATE;

typedef struct {
    uint32_t expire_time;
    char pkp_fingerprint[MAX_PING_NUM][FINGERPRINT_LENGTH + 1];
    FP_CACHE_STATE state;
} pkp_cache_t;

typedef struct {
    uint8_t client_num;
    char tls_fingerprint[MAX_CERTCHAIN_LENGTH][FINGERPRINT_LENGTH + 1];
    FP_CACHE_STATE state;
} tls_cache_t;

typedef struct {
    uint8_t domain[HTTPCLIENT_MAX_HOST_LENGTH + 1];
    pkp_cache_t pkp_cache;
    tls_cache_t tls_cache;
} pkp_ctxt_t;

/* HTTP CLIENT States */
typedef enum {
    HCS_INIT = 1,
    HCS_CONNECT_IN_PROG,
    HCS_CONNECTED,
    HCS_SENTREQ,
    HCS_RECVRESP,
    HCS_ENDING
} HTTPCLIENT_STATE;

/*supported http client methods */
typedef enum {
    HTTP_CLIENT_GET_CMD = 1,
    HTTP_CLIENT_HEAD_CMD,
    HTTP_CLIENT_POST_CMD,
    HTTP_CLIENT_PUT_CMD,
    HTTP_CLIENT_DELETE_CMD,
    HTTP_CLIENT_CONNECT_CMD,
    HTTP_CLIENT_PATCH_CMD,
} HTTPC_REQUEST_CMD_E;

typedef struct {
    int in_httpc_check;
    uint32_t timer_lasttime;
    void *lh; /* lock handle */
    uint32_t *httpc_sess;
} gbl_httpc_ctxt_t;

typedef uint32_t httpc_hdl_t;

typedef struct http_rsp_cont {
    uint32_t totalLen;
    uint32_t flag;
    uint8_t data[1];
} HTTP_RSP_CONT;

typedef struct {
    uint32_t length;
    uint32_t contentlength;
    uint32_t resp_code;
    const void *data;
} http_client_cb_resp_t;

typedef void (*http_client_cb_t)(void *arg, int32_t state, void *http_resp);
typedef enum {
    // Used by http client rx callback
    HTTPC_RX_ERROR_SERVER_CLOSED = -8, /*will reset session and reconnect before next request*/
    HTTPC_RX_ERROR_RX_PROCESS = -7,
    HTTPC_RX_ERROR_RX_HTTP_HEADER = -6,
    HTTPC_RX_ERROR_INVALID_RESPONSECODE = -5,
    HTTPC_RX_ERROR_CLIENT_TIMEOUT = -4,
    HTTPC_RX_ERROR_NO_BUFFER = -3,
    HTTPC_RX_CONNECTION_CLOSED = -2,
    HTTPC_RX_ERROR_CONNECTION_CLOSED = -1,
    HTTPC_RX_FINISHED = 0,
    HTTPC_RX_MORE_DATA = 1,
    HTTPC_RX_TUNNEL_ESTABLISHED = 2,
    HTTPC_RX_DATA_FROM_TUNNEL = 3,
    HTTPC_RX_TUNNEL_CLOSED = 4,
    HTTPC_RX_CHUNK_CONTINUE = 5,
} http_client_rx_cb_state;

typedef enum {
    // Used by http client return error
    HTTPC_ERR_SSL_NONE_CONF = -10,
    HTTPC_ERR_SOCKET_OPEN = -9,
    HTTPC_ERR_SSL_CONN = -8,
    HTTPC_ERR_INVALID_PARAM = -7,
    HTTPC_ERR_BUSY = -6,
    HTTPC_ERR_NO_MEMORY = -5,
    HTTPC_ERR_SEND = -4,
    HTTPC_ERR_CONN = -3,
    HTTPC_ERR_NONE_SESS = -2,
    HTTPC_ERROR = -1,
    HTTPC_OK = 0,
} http_client_error_code_e;

typedef enum {
    HTTP_CLIENT_RSP_NO_HEADER,
    HTTP_CLIENT_RSP_WITH_INVALID_FORMAT_HEADER,
    HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER,
} http_client_rsp_header_status;

typedef enum {
    HTTPS_NOT_AUTH,
    HTTPS_SERVER_AUTH,
    HTTPS_CLIENT_AUTH,
    HTTPS_BOTH_AUTH,
} https_auth_mode;

typedef struct http_client_rx_cb_s {
    uint8_t pending_req;
    int32_t state;
    uint32_t resp_code;
} rx_cb_t;

/**
 * @brief Secured connection context.
 */
typedef struct SSLContext {
    mbedtls_ssl_config config;               /**< @brief SSL connection configuration. */
    mbedtls_ssl_context context;             /**< @brief SSL connection context */
    mbedtls_x509_crt_profile certProfile;    /**< @brief Certificate security profile for this connection. */
    mbedtls_x509_crt rootCa;                 /**< @brief Root CA certificate context. */
    mbedtls_x509_crt clientCert;             /**< @brief Client certificate context. */
    mbedtls_pk_context privKey;              /**< @brief Client private key context. */
    mbedtls_entropy_context entropyContext;  /**< @brief Entropy context for random number generation. */
    mbedtls_ctr_drbg_context ctrDrbgContext; /**< @brief CTR DRBG context for random number generation. */
} SSLContext_t;

// Special value to indicate that no timeout is set
#define HTTPC_TIMEOUT_DISABLE (0xFFFFFFFF)

#define UNUSED(x)               (void)(x)
#define HTTPC_DOWNGRADE(sess)   (http_client_downgrade(sess))
#define HTTPC_IS_UPGRADED(sess) (sess->protocol_upgrade_module_rx_cb != NULL)

typedef void (*http_client_protocol_upgrade_module_rx_cb)(void *arg, void *session, uint8_t *p_rx_data,
                                                          uint32_t rx_data_length);

typedef struct httpclient_sess_s {
    uint8_t index;
    HTTPCLIENT_STATE hcs_state;
    int32_t hcs_socket;
    uint32_t hcs_flags;
    ip_addr_t hcs_addr;
    uint16_t hcs_port; /* TCP port for HTTP or HTTPS */
    uint8_t hcs_host[HTTPCLIENT_MAX_HOST_LENGTH +
                     1]; /* hostname of webserver or proxy, e.g. "www.example.com" or "192.168.2.100" */
    uint8_t hcs_url[HTTPCLIENT_MAX_URL_LENGTH + 1]; /* request-URL e.g. "/path/index.html" if hcs_host is an origin
                                                     * server or "www.example.com:22" if hcs_host is a proxy
                                                     */
    uint32_t isHttps;
    uint16_t is_pre_alccote_ssl_buffer; /* 0: don't pre allocte; 1:pre allocte, default: 0 */
    uint8_t https_auth_type;
    uint16_t ipprefer;
    HTTPC_REQUEST_CMD_E hcs_command;
    SSLContext_t *sslCtx;
    qapi_Ssl_Cert_t *sslCert;
    qapi_Ssl_Config_t *sslCfg;
    uint32_t timeout; /* timeout (in msec) for hcs_command */
    uint32_t hcs_lasttime;
    /*for data process*/
    uint32_t hcs_contentlength;
    uint32_t hcs_headersize;
    uint32_t hcs_rxbufoffset;
    uint8_t *hcs_rxbuffer;
    uint32_t hcs_rxbuf_len;
    uint8_t *hcs_buffer;
    uint32_t hcs_bufoffset;
    uint32_t hcs_buf_len;
    uint8_t *hcs_headerbuffer;
    uint8_t hcs_host_flag;
    uint8_t reserve1;
    uint8_t reserve2;
    uint8_t reserve3;
    uint32_t hcs_headerbufoffset;
    uint32_t hcs_headerbuf_len;
    uint32_t offset;
#ifdef HTTPC_DEBUG
    uint32_t total_chunks;
#endif
    uint32_t remain_len;
    /*for callback*/
    rx_cb_t cb;                      /* rx control block */
    http_client_cb_t http_client_cb; /* callback to return rx data */
    void *cb_arg;                    /* first arg of http_client_cb() */

    // An HTTP connection can be upgraded to a different protocol, e.g.
    // websocket, using the upgrade header.  If this flag is set, the
    // RX callback is given raw bytes rather than the parsed HTTP
    // response.
    void *protocol_upgrade_module_rx_cb_arg;
    http_client_protocol_upgrade_module_rx_cb protocol_upgrade_module_rx_cb;

} httpclient_sess;

int http_client_downgrade(httpclient_sess *sess);

/* Utility Macros */
#define HTTPC_MOVETOSTATE(sess, state)  (sess)->hcs_state = (state)
#define HTTPC_SET_FLAG(sess, flag)      (sess)->hcs_flags |= (flag)
#define HTTPC_RESET_FLAG(sess, flag)    (sess)->hcs_flags &= ~(flag)
#define HTTPC_FLAG_IS_SET(sess, flag)   (((sess)->hcs_flags & (flag)) ? TRUE : FALSE)
#define HTTPC_FLAG_IS_RESET(sess, flag) (!((sess)->hcs_flags & (flag)) ? TRUE : FALSE)

#define HTTP_RESPONSE_CODE_MIN (100)
#define HTTP_RESPONSE_CODE_MAX (999)

#define IS_IPV6_LINK_LOCAL(ipv6_Address)                                                \
    (((void *)ipv6_Address != NULL) && (((uint8_t *)ipv6_Address)[0] == 0xfe) &&        \
     (((uint8_t *)ipv6_Address)[1] == 0x80) && (((uint8_t *)ipv6_Address)[2] == 0x0) && \
     (((uint8_t *)ipv6_Address)[3] == 0x0) && (((uint8_t *)ipv6_Address)[4] == 0x0) &&  \
     (((uint8_t *)ipv6_Address)[5] == 0x0) && (((uint8_t *)ipv6_Address)[6] == 0x0) &&  \
     (((uint8_t *)ipv6_Address)[7] == 0x0))

int http_client_stop(void);
int http_client_start(void);
httpclient_sess *http_client_newsess(uint32_t timeout, uint32_t isHttps, http_client_cb_t callback, void *arg,
                                     uint16_t httpc_max_body_length, uint16_t httpc_max_header_length,
                                     uint16_t rxbufsize, uint16_t ip_prefer, uint16_t ssl_pre_buffer, uint8_t https_auth_type);
int http_client_freesess(httpclient_sess *sess);
int http_client_connect(httpclient_sess *sess, const char *server, uint16_t port);
int http_client_disconnect(httpclient_sess *sess);
int http_client_request(httpclient_sess *sess, HTTPC_REQUEST_CMD_E cmd, const char *url);
int http_client_tunnel_to_https(httpclient_sess *sess, const char *calist, const char *url);
int http_client_set_body(httpclient_sess *sess, const char *body, uint32_t bodylen);
int http_client_set_param(httpclient_sess *sess, const char *key, const char *value);
int http_client_add_header_field(httpclient_sess *sess, const char *header, const char *value);
int http_client_clear_header(httpclient_sess *sess);
int http_client_sslconfigure(httpclient_sess *sess, qapi_Ssl_Config_t *cfg);
int http_client_free_sslconfigure(httpclient_sess *sess);
int http_client_certconfigure(httpclient_sess *sess, qapi_Ssl_Cert_t *crt);
int http_client_free_sslcert(httpclient_sess *sess);
int http_client_cb_eable_adding_header(httpclient_sess *sess, int32_t enable);
int http_client_senddata(httpclient_sess *sess, char *buf, int length);
int http_client_send_chunk(httpclient_sess *sess, HTTPC_REQUEST_CMD_E cmd, const char *URL, const char *chunk_data,
                           int32_t chunk_size, uint8_t chunk_flag, int32_t total_size);
int http_client_release_pre_allcoate_buffer();

#endif /* _HTTPD_H_ */
