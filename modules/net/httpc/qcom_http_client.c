/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ctype.h"

#include "dns.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/err.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "data_path.h"
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "qurt_signal.h"
#include "task.h"

#include "qapi_status.h"
#include "qcom_http_client.h"
#include "qapi_heap_status.h"
#include "netif.h"
#include "network_al.h"

#ifdef HTTPC_DEBUG
#pragma push
#pragma O0
#endif

#ifdef CONFIG_QAT_OTA_DEMO
#include "ota_http.h"
extern http_session_info_t *ota_http_sess;
#endif

#ifdef CONFIG_QAT_OTA_DEMO
extern http_session_info_t *ota_http_sess;
#define htdbgprintf(...)                  \
    do { \
        if (!(ota_http_sess && ota_http_sess->status == HTTP_OTA_STATUS_RUNNING)) { \
            printf(__VA_ARGS__); \
        } \
    } while (0)
#else
#define htdbgprintf(...) printf(__VA_ARGS__)
#endif

TaskHandle_t th_httpc = NULL;
void http_client_task(void *pvParameters);
uint8_t g_httpc_task_priority = 6;
#ifdef CONFIG_QAT_HTTPC_DEMO
uint16_t g_httpc_task_stack_size = 1024;
#else
uint16_t g_httpc_task_stack_size = 1024 * 2;
#endif
uint32_t httpc_max_num_con = HTTPCLIENT_DEFAULT_CON_SUPPORT;
gbl_httpc_ctxt_t *g_httpc_ctxt = NULL;
uint8_t httpc_thread_started = FALSE;
qurt_signal_t httpc_sem;
#if CONFIG_QAT_POWERSAVE_DEMO
extern qurt_signal_t http_sem;
extern uint8_t powersave_active;
#endif

/* HTTP requests we support.
 * Do NOT change order of method strings.
 */
static char *http_req[] = {"", "GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "PATCH"};

static int http_client_resetsess(httpclient_sess *sess);
static int http_client_sslconnect(httpclient_sess *sess);
static int http_client_sendreq(httpclient_sess *sess);
static int http_client_rx_cb(httpclient_sess *sess, int32_t state, void *chunk_data, int32_t chunk_size);
static void sslContextFree(SSLContext_t *pSslContext);
static int http_client_processpkt(httpclient_sess *sess, int length);

static int cmp_char_i(unsigned char c1, unsigned char c2)
{
    /* Convert UC to LC */
    if (('A' <= c1) && ('Z' >= c1)) {
        c1 = c1 - 'A' + 'a';
    }
    if (('A' <= c2) && ('Z' >= c2)) {
        c2 = c2 - 'A' + 'a';
    }
    return (c1 - c2);
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    unsigned char c1, c2;
    int diff;

    if (n > 0) {
        do {
            c1 = (unsigned char)(*s1++);
            c2 = (unsigned char)(*s2++);
            diff = cmp_char_i(c1, c2);
            if (0 != diff) {
                return diff;
            }
            if ('\0' == c1) {
                break;
            }
        } while (--n);
    }
    return 0;
}

int http_client_downgrade(httpclient_sess *sess)
{
    if (!sess) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    sess->protocol_upgrade_module_rx_cb = NULL;
    sess->protocol_upgrade_module_rx_cb_arg = NULL;

    return HTTPC_OK;
}

/* Common static function for character processing
 */
static long get_chunksize(const char *hex)
{
    long ret = 0;
    char *errstr = NULL;

    if (hex == NULL)
        return -1;

    ret = strtol((const char *)hex, &errstr, 16);
    /* CR2383823: enven if there is no hex digits, still want to check if the length is less
       than zero. If less than zero, chunk_data pointer will be a invalid value and cause crash */
    if ((errstr == hex) || (ret < 0) /* there were no hex digits at all */
        || ((*errstr != '\0') && (*errstr != ' '))) {
        ret = -1;
    }
    return ret;
}

/***********************************************************************
 * Called when an HTTP tunnel is established.
 *
 * Start TLS handshake if the origin server is an HTTPS server.
 **********************************************************************/

static int setup_tunnel(httpclient_sess *sess)
{
    if (HTTPC_FLAG_IS_RESET(sess, HCF_TUNNEL_TO_HTTPS)) {
        /* For non-HTTPS origin server, we just return */
        return 0;
    }

    /* Delete an existent SSL connection */
    if (sess->sslCtx) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
    }

    return http_client_sslconnect(sess);
}

/***********************************************************************
 * To URL-encode a NULL-terminated string 'src'.
 * The encoded string (also NULL-terminated) is stored in 'dest' if 'dest' is not NULL.
 *
 * 1. unsafe characters -> %xx
 * 2. space -> + or %20
 * For example:
 * "Fred & Ethel" -> "Fred+%26+Ethel" or "Fred%20%26%20Ethel"
 *
 * RETURN: the length of the URL-encoded string
 **********************************************************************/
static uint32_t http_client_urlencode(uint8_t *src, uint8_t *dest)
{
    char *pstr = (char *)src;
    char *pbuf = (char *)dest;
    char tmp[1], *p;

    if (dest == NULL) {
        pbuf = tmp;
    }

    p = pbuf;

    while (*pstr) {
        if (isalnum((int)*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')  // ctype.h
        {
            if (dest != NULL) {
                *pbuf = *pstr;
            }
            pbuf++;
        } else if (*pstr == ' ') {
            if (dest != NULL) {
                *pbuf = '+';
            }
            pbuf++;
        } else {
            if (dest != NULL) {
                *pbuf = '%';
                snprintf(pbuf + 1, strlen(pbuf), "%2x", *pstr);
            }
            pbuf += 3;
        }
        pstr++;
    }
    if (dest != NULL) {
        *pbuf = '\0';
        htdbgprintf("%s() Copied string is %s Len:%u\n", __func__, dest, ((long)pbuf - (long)dest));
    }

    return (uint32_t)(pbuf - p);
}

/* FUNCTION: http_client_sess_is_found()
 *
 * check whether session can be found in httpc_sess array
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_sess_is_found(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    if (!sess || !g_httpc_ctxt)
        return HTTPC_ERR_NONE_SESS;

    qurt_mutex_lock(g_httpc_ctxt->lh);
    if (sess != (httpclient_sess *)g_httpc_ctxt->httpc_sess[sess->index])
        error = HTTPC_ERROR;

    qurt_mutex_unlock(g_httpc_ctxt->lh);
    return error;
}

/** DNS callback
 * If ipaddr is non-NULL, resolving succeeded and the request can be sent, otherwise it failed.
 */

int g_httpc_dns_found = 0;
static void httpc_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
    (void)arg;
    if (ipaddr) {
        if (arg) {
            memmove(arg, ipaddr, sizeof(ip_addr_t));
        }
        g_httpc_dns_found = 1;
        htdbgprintf("%s http host IP is  %s\r\n", hostname, ipaddr_ntoa(ipaddr));
    } else {
        g_httpc_dns_found = -1;
        htdbgprintf("get %s http host IP failed\n", hostname);
    }
}

/* FUNCTION: http_client_resolve()
 *
 * resolve hostname by dns module
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_resolve(httpclient_sess *sess)
{
    ip_addr_t ipaddr = {0};
    uint16_t count = 0;
    g_httpc_dns_found = 0;
    uint16_t ip_prefer = sess->ipprefer;
    uint16_t ip_prefer_check_num = 0;
    htdbgprintf("%s() Flags:0x%x\n", __func__, sess->hcs_flags);

    /* Find port no */
    if (!sess->hcs_port) {
        sess->hcs_port = 80;
    }

    while (1) {
        if (ip_prefer == IP_V4) {
            htdbgprintf("IPv4 check\n");
#if LWIP_IPV4

            /* Check whether it is IPv4 address */
#if LWIP_IPV6
            if ((inet_pton(AF_INET, (char *)sess->hcs_host, (void *)&(sess->hcs_addr.u_addr.ip4))) == 1)
#else
            if ((inet_pton(AF_INET, (char *)sess->hcs_host, (void *)&(sess->hcs_addr))) == 1)
#endif
            {
                /* Success */
#if LWIP_IPV6
                htdbgprintf("%s() IPv4 Addr: 0x%08x\n", __func__, sess->hcs_addr.u_addr.ip4);
                sess->hcs_addr.type = AF_INET;
#else
                htdbgprintf("%s() IPv4 Addr: 0x%08x\n", __func__, sess->hcs_addr);
#endif
                return HTTPC_OK;
            }
#endif
            ip_prefer = IP_V6;
            ip_prefer_check_num++;
            if (ip_prefer_check_num == 2)
                break;
        }

        if (ip_prefer == IP_V6) {
            htdbgprintf("IPv6 check\n");
#if LWIP_IPV6
            /* Check whether it is IPv6 address */
#if LWIP_IPV4
            if ((inet_pton(AF_INET6, (char *)sess->hcs_host, (void *)&(sess->hcs_addr.u_addr.ip6))) == 1)
#else
            if ((inet_pton(AF_INET6, (char *)sess->hcs_host, (void *)&(sess->hcs_addr))) == 1)
#endif
            {
                char temp[46];
                inet_ntop(AF_INET6, (void *)&(sess->hcs_addr.u_addr.ip6), temp, sizeof(temp));
                /* Success */
                // htdbgprintf("%s() IPv6 Addr:%s\n", __func__, inet_ntop(AF_INET6, (void *)&(sess->hcs_addr.a.addr6),
                // temp,sizeof(temp)));
#if LWIP_IPV4
                htdbgprintf("%s() IPv6 Addr:%s\n", __func__, temp);

                sess->hcs_addr.type = AF_INET6;
#else
                htdbgprintf("%s() IPv6 Addr: 0x%08x\n", __func__, sess->hcs_addr);
#endif
                return HTTPC_OK;
            }
#endif
            ip_prefer = IP_V4;
            ip_prefer_check_num++;
            if (ip_prefer_check_num == 2)
                break;
        }
    }
    /* We will try to use IPV4 dns server if present
     * Only if IPv6 DNS server is alone there, use IPv6 DNS server
     */

    htdbgprintf("%s(): need dns to resolve.\n", __func__);
    // err_enum_t err = dns_gethostbyname((char *)sess->hcs_host, &ipaddr, httpc_dns_found, &ipaddr);
    u8_t dns_addrtype = LWIP_DNS_ADDRTYPE_DEFAULT;
#if LWIP_IPV4 && LWIP_IPV6
    if (sess->ipprefer == IP_V6)
        dns_addrtype = LWIP_DNS_ADDRTYPE_IPV6_IPV4;
#endif
    htdbgprintf("dns_addrtype:%d\n", dns_addrtype);
    err_enum_t err =
        dns_gethostbyname_addrtype((char *)sess->hcs_host, &ipaddr, httpc_dns_found, &ipaddr, dns_addrtype);

    if (err == ERR_OK) {
        g_httpc_dns_found = 1;
    } else if (err == ERR_INPROGRESS) {
        while ((g_httpc_dns_found == 0) && (count++ < 5)) {
            qurt_thread_sleep(1000);
        }
    }

    if (g_httpc_dns_found == 1) {
#if LWIP_IPV4 && LWIP_IPV6
        htdbgprintf("get ip_addr type: %d\n", ipaddr.type);
        // the type is different for static/dynamic DNS resolved
        if (ipaddr.type == LWIP_DNS_ADDRTYPE_IPV4 || ipaddr.type == IPADDR_TYPE_V4) {
            sess->hcs_addr.type = AF_INET;
            sess->hcs_addr.u_addr.ip4 = ipaddr.u_addr.ip4; /* in network order */
        } else if (ipaddr.type == LWIP_DNS_ADDRTYPE_IPV6 || ipaddr.type == IPADDR_TYPE_V6) {
            sess->hcs_addr.type = AF_INET6;
            memcpy(&sess->hcs_addr.u_addr.ip6, &ipaddr.u_addr.ip6, sizeof(ip6_addr_t));
        }
#else
        sess->hcs_addr = ipaddr; /* in network order */
#endif

        return HTTPC_OK;
    }

    return HTTPC_ERR_CONN;
}

/* FUNCTION: http_client_start()
 *
 * start HTTP client task and allocate memory
 *
 * PARAMS: void
 *
 * RETURNS: OK or ERROR code
 */
int http_client_start(void)
{
    if ((th_httpc != NULL) || (httpc_thread_started == TRUE)) {
        htdbgprintf("HTTPC is already running\n");
        return HTTPC_OK;
    }

    if (g_httpc_ctxt == NULL) {
        uint8_t client_list_size = 0;
        if ((g_httpc_ctxt = malloc(sizeof(gbl_httpc_ctxt_t))) == NULL) {
            htdbgprintf("HTTPC malloc gbl_httpc_ctxt_t fail\n");
            goto ERROR;
        }
        memset(g_httpc_ctxt, 0, sizeof(gbl_httpc_ctxt_t));
        /* Create a lock */
        if ((g_httpc_ctxt->lh = malloc(sizeof(qurt_mutex_t))) == NULL) {
            htdbgprintf("HTTPC malloc qurt_mutex_t fail\n");
            goto ERROR;
        }
        if ((qurt_mutex_create(g_httpc_ctxt->lh)) != QURT_EOK) {
            htdbgprintf("HTTPC qurt_mutex_create fail\n");
            goto ERROR;
        }

        client_list_size = sizeof(uint32_t) * (httpc_max_num_con);
        if ((g_httpc_ctxt->httpc_sess = malloc(client_list_size)) == NULL) {
            htdbgprintf("HTTPC malloc client_list_size fail\n");
            goto ERROR;
        }
        memset(g_httpc_ctxt->httpc_sess, 0, client_list_size);

        qurt_signal_create(&httpc_sem);

        /* Create httpclient task */

        if (th_httpc == NULL) {
            if (pdPASS != nt_qurt_thread_create(http_client_task, "httpc", g_httpc_task_stack_size, NULL,
                                                g_httpc_task_priority, &th_httpc)) {
                htdbgprintf("HTTPC thread create fail\n");
                goto ERROR;
            }
        }

        httpc_thread_started = TRUE;

        return HTTPC_OK;

    ERROR:
        http_client_stop();
        htdbgprintf("HTTPC: Init failed\n");
        return HTTPC_ERR_NO_MEMORY;
    }
    return HTTPC_OK;
}

/* FUNCTION: http_client_stop()
 *
 * stop HTTP client task and free memory
 *
 * PARAMS: void
 *
 * RETURNS: OK or ERROR code
 */
int http_client_stop(void)
{
    uint32 i;
    uint32 Signal_Waiting;

    if (g_httpc_ctxt != NULL) {
        if (httpc_thread_started == TRUE) {
            httpc_thread_started = FALSE;
            qurt_signal_wait_timed(&httpc_sem, 1, QURT_SIGNAL_ATTR_CLEAR_MASK, &Signal_Waiting, QURT_TIME_WAIT_FOREVER);
        }
        if (g_httpc_ctxt->httpc_sess) {
            for (i = 0; i < httpc_max_num_con; i++) {
                if (g_httpc_ctxt->httpc_sess[i] != 0) {
                    http_client_freesess((httpclient_sess *)(g_httpc_ctxt->httpc_sess[i]));
                }
            }
            free(g_httpc_ctxt->httpc_sess);
        }
        if (g_httpc_ctxt->lh) {
            qurt_mutex_delete((qurt_mutex_t *)g_httpc_ctxt->lh);
            free(g_httpc_ctxt->lh);
            g_httpc_ctxt->lh = NULL;
        }
        free(g_httpc_ctxt);
        g_httpc_ctxt = NULL;
        if (httpc_sem != 0) {
            qurt_signal_delete(&httpc_sem);
        }

        nt_osal_thread_delete(th_httpc);
        th_httpc = NULL;
    }
    return (HTTPC_OK);
}

/* FUNCTION: http_client_release_pre_allcoate_buffer()
 *
 * release pre_allcoate buffer used for SSL
 * assignment inputs
 *
 * PARAMS:void
 *
 * RETURNS: OK or ERROR code
 */
int http_client_release_pre_allcoate_buffer()
{
    htdbgprintf("release SSL buffer,in_buf len:%d, out_buf len:%d\n", pre_ssl_in_buffer_len, pre_ssl_out_buffer_len);
    pre_allocte_big_memory = 0;
    if (pre_ssl_in_buffer) {
        memset(pre_ssl_in_buffer, 0, pre_ssl_in_buffer_len);
        // free(pre_ssl_in_buffer);
        mbedtls_zeroize_and_free(pre_ssl_in_buffer, pre_ssl_in_buffer_len);
    }
    pre_ssl_in_buffer = NULL;
    pre_ssl_in_buffer_len = 0;

    if (pre_ssl_out_buffer) {
        memset(pre_ssl_out_buffer, 0, pre_ssl_out_buffer_len);
        // free(pre_ssl_out_buffer);
        mbedtls_zeroize_and_free(pre_ssl_out_buffer, pre_ssl_out_buffer_len);
    }
    pre_ssl_out_buffer = NULL;
    pre_ssl_out_buffer_len = 0;
}

/* FUNCTION: http_client_newsess()
 *
 * new a session and mark in httpc_sess array
 * assignment inputs
 *
 * PARAMS: timeout, ssl_ctx, callback, arg, httpc_max_body_length, httpc_max_header_length
 *
 * RETURNS: pointer of session
 */
httpclient_sess *http_client_newsess(uint32_t timeout, uint32_t isHttps, http_client_cb_t callback, void *arg,
                                     uint16_t httpc_max_body_length, uint16_t httpc_max_header_length,
                                     uint16_t rxbufsize, uint16_t ip_prefer, uint16_t ssl_pre_buffer, uint8_t https_auth_type)
{
    httpclient_sess *session = NULL;
    uint32 i;

    if (g_httpc_ctxt == NULL)
        return NULL;

    qurt_mutex_lock(g_httpc_ctxt->lh);

    for (i = 0; i < httpc_max_num_con; i++) {
        if (g_httpc_ctxt->httpc_sess[i] == 0) {
            break;
        }
    }
    if (i >= httpc_max_num_con) {
        qurt_mutex_unlock(g_httpc_ctxt->lh);
        return NULL;
    }

    if ((session = malloc(sizeof(httpclient_sess))) == NULL) {
        qurt_mutex_unlock(g_httpc_ctxt->lh);
        htdbgprintf("HTTPC malloc httpclient_sess fail\n");
        return NULL;
    }
    memset(session, 0, sizeof(httpclient_sess));
    g_httpc_ctxt->httpc_sess[i] = (uint32_t)session;
    session->index = i;
    session->hcs_socket = INVALID_SOCKET;
    session->isHttps = isHttps;
    session->is_pre_alccote_ssl_buffer = ssl_pre_buffer;
    session->https_auth_type = https_auth_type;
    session->timeout = timeout;
    session->http_client_cb = callback;
    session->cb_arg = arg;
    session->hcs_buf_len = httpc_max_body_length;
    session->hcs_headerbuf_len = httpc_max_header_length;
    session->ipprefer = ip_prefer;
    qurt_mutex_unlock(g_httpc_ctxt->lh);

    htdbgprintf("HTTPC timeout:%d,ip prefer:%d,is_pre_buffer:%d\n", session->timeout, session->ipprefer,
                ssl_pre_buffer);

    if ((session->hcs_buffer = malloc(session->hcs_buf_len)) == NULL) {
        htdbgprintf("HTTPC malloc hcs_buffer fail\n");
        goto ERROR;
    }
    if ((session->hcs_headerbuffer = malloc(session->hcs_headerbuf_len)) == NULL) {
        htdbgprintf("HTTPC malloc hcs_headerbuffer fail\n");
        goto ERROR;
    }

    rxbufsize = max(rxbufsize, HTTPCLIENT_MIN_BUFFER_SIZE);
    session->hcs_rxbuf_len = rxbufsize;
    if ((session->hcs_rxbuffer = malloc(session->hcs_rxbuf_len)) == NULL) {
        htdbgprintf("HTTPC malloc hcs_rxbuffer fail\n");
        goto ERROR;
    }

    // clear all data
    memset(session->hcs_buffer, 0, session->hcs_buf_len);
    memset(session->hcs_headerbuffer, 0, session->hcs_headerbuf_len);
    memset(session->hcs_rxbuffer, 0, session->hcs_rxbuf_len);
    HTTPC_MOVETOSTATE(session, HCS_INIT);
    return session;

ERROR:
    http_client_freesess(session);
    return NULL;
}

/* FUNCTION: http_client_freesess()
 *
 * free HTTP client session, disconnect and free memory
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_freesess(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    qurt_mutex_lock(g_httpc_ctxt->lh);
    http_client_disconnect(sess);
    g_httpc_ctxt->httpc_sess[sess->index] = 0;

    /* Free the session */
    if (sess->hcs_buffer)
        free(sess->hcs_buffer);

    if (sess->hcs_headerbuffer)
        free(sess->hcs_headerbuffer);

    if (sess->hcs_rxbuffer)
        free(sess->hcs_rxbuffer);

    if (sess->sslCtx) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
    }

    free(sess);
    qurt_mutex_unlock(g_httpc_ctxt->lh);
    return HTTPC_OK;
}

/* FUNCTION: http_client_connect()
 *
 * connect HTTP server, blocking mode
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_connect(httpclient_sess *sess, const char *server, uint16_t port)
{
    int32_t error = HTTPC_OK;
#if LWIP_IPV4
    struct sockaddr_in s_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 s_addr6;
#endif
    struct sockaddr *to;
    uint32_t tolen;
    int32_t sock;
    int family;

    /*first check whether the session is in recording */
    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (sess->hcs_state > HCS_INIT) {
        htdbgprintf("%s():%d State:%u Flags:0x%x Soc:%d\n", __func__, __LINE__, sess->hcs_state, sess->hcs_flags,
                    sess->hcs_socket);
        return HTTPC_ERR_BUSY;
    }

    HTTPC_MOVETOSTATE(sess, HCS_CONNECT_IN_PROG);
    strlcpy((char *)sess->hcs_host, server, sizeof(sess->hcs_host));
    sess->hcs_port = (uint32_t)port;

    error = http_client_resolve(sess);
    if (error != HTTPC_OK) {
        goto ERROR;
    }

#if LWIP_IPV4 && LWIP_IPV6
    family = sess->hcs_addr.type;
#elif LWIP_IPV4
    family = AF_INET;
#elif LWIP_IPV6
    family = AF_INET6;
#endif
    htdbgprintf("family:%d\n", family);
    /* Create a socket if it is not created already */
    if (sess->hcs_socket == INVALID_SOCKET) {
        sock = socket(family, SOCK_STREAM, 0);

        if (sock == INVALID_SOCKET) {
            htdbgprintf("Socket create failed\n");
            error = HTTPC_ERR_SOCKET_OPEN;
            goto ERROR;
        }
        sess->hcs_socket = sock;
        htdbgprintf("Socket create ok, sock:%d.\n", sess->hcs_socket);
    }
    sock = sess->hcs_socket;
    setsockopt(sess->hcs_socket, SOL_SOCKET, O_NONBLOCK, NULL, 0);

#if LWIP_IPV4
    if (AF_INET == family) {
#if LWIP_IPV6

        htdbgprintf("%s() Addr 0x%08x port %u\n", __func__, sess->hcs_addr.u_addr.ip4, sess->hcs_port);
#else
        htdbgprintf("%s() Addr 0x%08x port %u\n", __func__, sess->hcs_addr, sess->hcs_port);
#endif
        memset(&s_addr, 0, sizeof(struct sockaddr_in));
#if LWIP_IPV6
        s_addr.sin_addr.s_addr = sess->hcs_addr.u_addr.ip4.addr;
#else
        s_addr.sin_addr.s_addr = sess->hcs_addr.addr;
#endif
        s_addr.sin_port = htons(sess->hcs_port);
        s_addr.sin_family = family;
        s_addr.sin_len = sizeof(struct sockaddr_in);
        to = (struct sockaddr *)&s_addr;
        tolen = sizeof(s_addr);

    } else {
#endif
#if LWIP_IPV6
        if (AF_INET6 == family) {
            memset(&s_addr6, 0, sizeof(struct sockaddr_in6));
            s_addr6.sin6_family = family;
#if LWIP_IPV4
            memscpy(&s_addr6.sin6_addr, sizeof(struct in6_addr), &sess->hcs_addr.u_addr.ip6, sizeof(ip6_addr_t));
#else
            memscpy(&s_addr6.sin6_addr, sizeof(struct in6_addr), &sess->hcs_addr, sizeof(ip6_addr_t));
#endif
            if (IS_IPV6_LINK_LOCAL(s_addr6.sin6_addr.un.u8_addr)) {
                /* if this is a link local address, then the interface must be found */

                if (nt_get_netifidx_by_devmode(STA_DEVICE)) {
                    s_addr6.sin6_scope_id = nt_get_netifidx_by_devmode(STA_DEVICE);
                    htdbgprintf("%s() sta_dev netif interface %d\n", __func__, s_addr6.sin6_scope_id);
                }

                else if (nt_get_netifidx_by_devmode(AP_DEVICE)) {
                    s_addr6.sin6_scope_id = nt_get_netifidx_by_devmode(AP_DEVICE);
                    htdbgprintf("%s() ap_dev netif interface %d\n", __func__, s_addr6.sin6_scope_id);
                } else {
                    htdbgprintf("network interface not initialized\r\n");
                    goto ERROR;
                }

#if 0
            /* if this is a link local address, then the interface must be specified after % */

            char * interface_name_with_percent_char = strchr(server, '%');
            char * interface_name = NULL;

            if ( interface_name_with_percent_char )
            {
                htdbgprintf("%s() interface %s\n", __func__, interface_name_with_percent_char);
                interface_name = interface_name_with_percent_char + 1;
                htdbgprintf("%s() interface %s\n", __func__, interface_name);
            }
            else
            {
                htdbgprintf("Link local IPv6 address is used, must append %%interface_name to the address\n");
                error = HTTPC_ERR_INVALID_PARAM;
                goto ERROR;
            }
#endif
            }
            s_addr6.sin6_port = htons(sess->hcs_port);
            to = (struct sockaddr *)&s_addr6;
            tolen = sizeof(s_addr6);
            // htdbgprintf("%s() %d IPv6 Addr:%s port %u\n", __func__, __LINE__, inet_ntop(AF_INET6, (void
            // *)&(sess->hcs_addr.u_addr.ip6)), sess->hcs_port);
        } else
#endif
        {
            htdbgprintf("%s():%d fatal error on index[%d]\n", __func__, __LINE__, sess->index);
            error = HTTPC_ERR_INVALID_PARAM;
            goto ERROR;
        }
#if LWIP_IPV4
    }
#endif
    htdbgprintf("%s():%d Sending Connect req on index[%d]\n", __func__, __LINE__, sess->index);

    error = connect(sock, to, tolen);

    if (error) {
        htdbgprintf("t_connect failure Err:%d\n", error);
        error = HTTPC_ERR_CONN;
        goto ERROR;
    }
#if 0
    //test begin
    heap_status hs={0};
    if(qapi_Heap_Status(&hs) != QAPI_OK)
    {
        printf("test ask qapi_Heap_Status fail.\n");
    }
    else{
        printf("test before ssl heap INFO:%d,%d,%d,%d \n",hs.total_Bytes, hs.total_Bytes-hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes);
    }
    //test end
#endif

    if (sess->isHttps)
        error = http_client_sslconnect(sess);

#if 0
    //test begin
    if(qapi_Heap_Status(&hs) != QAPI_OK)
    {
        printf("test ask qapi_Heap_Status fail.\n");
    }
    else{
        printf("test after ssl heap INFO:%d,%d,%d,%d \n",hs.total_Bytes, hs.total_Bytes-hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes);
    }
    //test end
#endif
    if (error != HTTPC_OK) {
        htdbgprintf("SSL connect failed\n");
        goto ERROR;
    }

    sess->hcs_lasttime = HTTPCTICKS;
    HTTPC_MOVETOSTATE(sess, HCS_CONNECTED);
    return HTTPC_OK;

ERROR:
    sess->hcs_lasttime = HTTPCTICKS;
    http_client_disconnect(sess);
    return error;
}

/* FUNCTION: http_client_disconnect()
 *
 * disconnect from HTTP server
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_disconnect(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    /*first check whether the seession is in recording */
    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;
    /*
     * move http session state to HCS_ENDING. In some case, client may wait SSL shutdown.
     * client also locked the httpc handle, as the result, other thread must wait the handle.
     */
    HTTPC_MOVETOSTATE(sess, HCS_ENDING);

    if (sess->sslCtx) {
        do {
            error = mbedtls_ssl_close_notify(&(sess->sslCtx->context));

        } while (error == MBEDTLS_ERR_SSL_WANT_WRITE);

        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
    }

    /* If the HTTP session wad upgraded to another protocol, e.g. Websocket
     * reset it back to HTTP.
     */
    if (HTTPC_IS_UPGRADED(sess)) {
        HTTPC_DOWNGRADE(sess);
    }

    /* If socket is not invalid, we need to close it even if it is not connected.
       Otherwise, session is freed and we will leak the sockets */
    if (sess->hcs_socket != INVALID_SOCKET) {
        if (HTTPC_ERROR == closesocket(sess->hcs_socket)) {
            htdbgprintf("%s() SOCKETCLOSE failed\n", __func__);
        }
        sess->hcs_socket = INVALID_SOCKET;
    }

    http_client_resetsess(sess);
    sess->hcs_lasttime = HTTPCTICKS;
    HTTPC_MOVETOSTATE(sess, HCS_INIT);
    return HTTPC_OK;
}

/* FUNCTION: http_client_request()
 *
 * process request and send it to HTTP server
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_request(httpclient_sess *sess, HTTPC_REQUEST_CMD_E cmd, const char *url)
{
    int32_t error = HTTPC_OK;
    rx_cb_t *cb;

    htdbgprintf("%s() Command: %d\t url: %s\n", __func__, cmd, url);

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (cmd < HTTP_CLIENT_GET_CMD || cmd > HTTP_CLIENT_PATCH_CMD) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    cb = &(sess->cb);

    /* Error: Have NOT got the response for previous HTTP request. */
    if (cb->pending_req) {
        return HTTPC_ERR_BUSY;
    }

    /* Reconnect to HTTP server */
    if (HTTPC_FLAG_IS_SET(sess, HCF_CLOSED_BY_PEER) && (sess->hcs_state == HCS_INIT)) {
        http_client_connect(sess, (const char *)sess->hcs_host, sess->hcs_port);
    }

    if (sess->hcs_state != HCS_CONNECTED) {
        htdbgprintf("error status %d.\n", sess->hcs_state);
        if (sess->hcs_state < HCS_CONNECTED)
            return HTTPC_ERR_CONN;
        else
            return HTTPC_ERR_BUSY;
    }

    HTTPC_RESET_FLAG(sess, HCF_CLOSED_BY_PEER);

    if (cmd == HTTP_CLIENT_CONNECT_CMD) {
        /* www.example.com:22 -or-
         * 192.168.2.100:80   -or-
         * [2001:db8:85a3:8d3:1319:8a2e:370:7348]:443
         */
        strlcpy((char *)(sess->hcs_url), url, sizeof(sess->hcs_url));
    } else {
        /* Copy the page name URI and make it an absolute path */
        if (!url) {
            sess->hcs_url[0] = '/';
        } else if (*url != '/') {
            sess->hcs_url[0] = '/';
            strlcpy((char *)(sess->hcs_url + 1), url, sizeof(sess->hcs_url) - 1);
        } else
            strlcpy((char *)(sess->hcs_url), url, sizeof(sess->hcs_url));
    }

    sess->hcs_command = cmd;

    error = http_client_sendreq(sess);
    if (HTTPC_OK == error) {
        sess->hcs_lasttime = HTTPCTICKS;
    }
    return error;
}

/***********************************************************************
 * url: "<host>:<port>"
 * e.g. "www.example.com:443" -or-
 *      "[2001:db8:85a3:8d3:1319:8a2e:370:7348]:1443"
 **********************************************************************/
#ifdef HTTP_TUNNEL
int http_client_tunnel_to_https(httpclient_sess *sess, const char *calist, const char *url)
{
    int32_t error;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK) {
        return error;
    }
#ifdef HTTP_TUNNEL
    if (calist != NULL && calist[0] != '\0') {
        size_t len = strlen(calist);

        sess->calist_tun = malloc(len + 1);
        if (sess->calist_tun == NULL) {
            return HTTPC_ERR_NO_MEMORY;
        }
        strlcpy(sess->calist_tun, calist, len);
    }
    HTTPC_SET_FLAG(sess, HCF_TUNNEL_TO_HTTPS);
#endif
    return http_client_request(sess, HTTP_CLIENT_CONNECT_CMD, url);
}
#endif

/* FUNCTION: http_client_set_body()
 *
 * Copy 'bodylen' bytes of data from 'body' to hcs_buffer.
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_set_body(httpclient_sess *sess, const char *body, uint32_t bodylen)
{
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (bodylen > sess->hcs_buf_len || body == NULL) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    sess->hcs_lasttime = HTTPCTICKS;

    memset(sess->hcs_buffer, 0, sess->hcs_buf_len);
    memcpy(sess->hcs_buffer, body, bodylen);
    sess->hcs_bufoffset = bodylen;

    return HTTPC_OK;
}

/* FUNCTION: http_client_set_param()
 *
 * Form 'key1=value1&key2=value2&...' in hcs_buffer. For example:
 *  name=Lucy&neighbors=Fred+%26+Ethel
 *
 * PARAMS: sess         session handle
 *         key, value   NULL-terminated string
 *
 * RETURNS: OK or ERROR code
 */
int http_client_set_param(httpclient_sess *sess, const char *key, const char *value)
{
    uint32_t len;
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (!key || !value) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    /* Check if hcs_buffer is long enough to contain URL-encoded key=value string */
    len = 1; /* for '=' */
    if (sess->hcs_bufoffset != 0) {
        ++len; /* for non-first name/value pair, we need to add '&' */
    }
    len += http_client_urlencode((uint8_t *)key, NULL);
    len += http_client_urlencode((uint8_t *)value, NULL);
    if (len >= sess->hcs_buf_len - sess->hcs_bufoffset) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    sess->hcs_lasttime = HTTPCTICKS;

    /* For first name/value pair, no need to add '&'*/
    if (sess->hcs_bufoffset != 0) {
        *(sess->hcs_buffer + sess->hcs_bufoffset) = '&';
        sess->hcs_bufoffset += 1;
    }

    len = http_client_urlencode((uint8_t *)key, sess->hcs_buffer + sess->hcs_bufoffset);
    sess->hcs_bufoffset += len;

    *(sess->hcs_buffer + sess->hcs_bufoffset) = '=';
    sess->hcs_bufoffset += 1;

    len = http_client_urlencode((uint8_t *)value, sess->hcs_buffer + sess->hcs_bufoffset);
    sess->hcs_bufoffset += len;

    htdbgprintf("Data is %s sess_bufoffset:%u\n", sess->hcs_buffer, sess->hcs_bufoffset);
    return HTTPC_OK;
}

/* FUNCTION: http_client_add_header_field()
 *
 * add header to sess
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_add_header_field(httpclient_sess *sess, const char *header, const char *value)
{
    int32_t str_len;
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    /* If data or url is not present */
    if (!header || !value) {
        return HTTPC_ERR_INVALID_PARAM;
    }

    str_len = strlen(header) + 2 + /* for ": " */
              strlen(value) + 2;   /* for "\r\n" */
    if ((str_len + sess->hcs_headerbufoffset) >= sess->hcs_headerbuf_len) {
        htdbgprintf("http header can't exceed head buffer length, please clear header\n");
        return HTTPC_ERR_INVALID_PARAM;
    }

    if (0 == strncmp(header, "Host", 4)) {
        sess->hcs_host_flag = 1;
    }
    sess->hcs_lasttime = HTTPCTICKS;
    sess->hcs_headerbufoffset += snprintf((char *)(sess->hcs_headerbuffer + sess->hcs_headerbufoffset),
                                          sess->hcs_headerbuf_len - sess->hcs_headerbufoffset, "%s", header);
    sess->hcs_headerbufoffset += snprintf((char *)(sess->hcs_headerbuffer + sess->hcs_headerbufoffset),
                                          sess->hcs_headerbuf_len - sess->hcs_headerbufoffset, ": ");
    sess->hcs_headerbufoffset += snprintf((char *)(sess->hcs_headerbuffer + sess->hcs_headerbufoffset),
                                          sess->hcs_headerbuf_len - sess->hcs_headerbufoffset, "%s", value);
    sess->hcs_headerbufoffset += snprintf((char *)(sess->hcs_headerbuffer + sess->hcs_headerbufoffset),
                                          sess->hcs_headerbuf_len - sess->hcs_headerbufoffset, "\r\n");

    htdbgprintf("Data is %s sess_headerbufoffset:%u\n", sess->hcs_headerbuffer, sess->hcs_headerbufoffset);

    return HTTPC_OK;
}

/* FUNCTION: http_client_clear_header()
 *
 * clear header to sess
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_clear_header(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    sess->hcs_lasttime = HTTPCTICKS;

    /* Make it zero so that the next GET/POST req will be clean */
    memset(sess->hcs_headerbuffer, 0, sess->hcs_headerbuf_len);
    sess->hcs_headerbufoffset = 0;
    sess->hcs_host_flag = 0;

    return HTTPC_OK;
}

/* FUNCTION: http_client_resetsess()
 *
 * clean member in HTTP client session
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_resetsess(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;
    qbool_t conn_closed_by_peer;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    conn_closed_by_peer = HTTPC_FLAG_IS_SET(sess, HCF_CLOSED_BY_PEER);
    if (!conn_closed_by_peer) {
        memset(sess->hcs_host, 0, sizeof(sess->hcs_host));
        sess->hcs_port = 0;
        memset(sess->hcs_headerbuffer, 0, sess->hcs_headerbuf_len);
        sess->hcs_headerbufoffset = 0;
        sess->hcs_host_flag = 0;
    }

    sess->hcs_flags = 0;
    /* keep HCF_CLOSED_BY_PEER flag */
    if (conn_closed_by_peer) {
        HTTPC_SET_FLAG(sess, HCF_CLOSED_BY_PEER);
    }

    memset(&sess->hcs_addr, 0, sizeof(sess->hcs_addr));
    memset(sess->hcs_url, 0, sizeof(sess->hcs_url));
    sess->hcs_command = (HTTPC_REQUEST_CMD_E)0;
    sess->hcs_contentlength = 0;
    sess->hcs_headersize = 0;
    sess->hcs_rxbufoffset = 0;
    sess->hcs_bufoffset = 0;
    sess->offset = 0;
    memset(&sess->cb, 0, sizeof(sess->cb));
    sess->remain_len = 0;
    memset(sess->hcs_buffer, 0, sess->hcs_buf_len);
    memset(sess->hcs_rxbuffer, 0, sess->hcs_rxbuf_len);

    return error;
}

static int sslContextInit(SSLContext_t *pSslContext)
{
    int mbedtlsError = 0;
    const char *pers = "https";

    configASSERT(pSslContext != NULL);

    mbedtls_ssl_config_init(&(pSslContext->config));
    mbedtls_x509_crt_init(&(pSslContext->rootCa));
    mbedtls_pk_init(&(pSslContext->privKey));
    mbedtls_x509_crt_init(&(pSslContext->clientCert));
    mbedtls_ssl_init(&(pSslContext->context));
    mbedtls_entropy_init(&(pSslContext->entropyContext));
    mbedtls_ctr_drbg_init(&(pSslContext->ctrDrbgContext));

    mbedtlsError = mbedtls_ctr_drbg_seed(&(pSslContext->ctrDrbgContext), mbedtls_entropy_func,
                                         &(pSslContext->entropyContext), (const unsigned char *)pers, strlen(pers));

    if (mbedtlsError != 0) {
        return -1;
    }

    return 0;
}

static void sslContextFree(SSLContext_t *pSslContext)
{
    configASSERT(pSslContext != NULL);

    // mbedtls_ssl_free( &( pSslContext->context ) );
    mbedtls_ssl_free_pre_allocate(&(pSslContext->context));
    mbedtls_x509_crt_free(&(pSslContext->rootCa));
    mbedtls_x509_crt_free(&(pSslContext->clientCert));
    mbedtls_pk_free(&(pSslContext->privKey));
    mbedtls_entropy_free(&(pSslContext->entropyContext));
    mbedtls_ctr_drbg_free(&(pSslContext->ctrDrbgContext));
    mbedtls_ssl_config_free(&(pSslContext->config));
    free(pSslContext);
}

static int sslSetCredentials(httpclient_sess *sess, SSLContext_t *pSslContext)
{
    int32_t mbedtlsError = -1;
    if (!sess) {
        return -1;
    }
    if (sess->sslCert == NULL && sess->https_auth_type != HTTPS_NOT_AUTH) {
        return -1;
    }
    if (sess->https_auth_type == HTTPS_NOT_AUTH) {
        mbedtls_ssl_conf_authmode(&(pSslContext->config), MBEDTLS_SSL_VERIFY_NONE);
    } else {
        mbedtls_ssl_conf_authmode(&(pSslContext->config), MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    mbedtls_ssl_conf_rng(&(pSslContext->config), mbedtls_ctr_drbg_random, &(pSslContext->ctrDrbgContext));
    if (sess->https_auth_type != HTTPS_NOT_AUTH) {
        // set root ca
        if (sess->sslCert->pRootCa) {
            mbedtlsError = mbedtls_x509_crt_parse(&(pSslContext->rootCa), (const unsigned char *)sess->sslCert->pRootCa,
                                                sess->sslCert->rootCaSize);
        }
        if (mbedtlsError != 0) {
            htdbgprintf("%s:%d: set rootca fail.\n", __func__, __LINE__);
            return -1;
        }

        mbedtls_ssl_conf_ca_chain(&(pSslContext->config), &(pSslContext->rootCa), NULL);
        // set client cert
        if (sess->sslCert->pClientCert) {
            mbedtlsError =
                mbedtls_x509_crt_parse(&(pSslContext->clientCert), (const unsigned char *)sess->sslCert->pClientCert,
                                    sess->sslCert->clientCertSize);
        }
        if (mbedtlsError != 0) {
            htdbgprintf("%s:%d: set client crt fail.\n", __func__, __LINE__);
            return -1;
        }
        // set pk
        if (sess->sslCert->pPrivateKey) {
            mbedtlsError = mbedtls_pk_parse_key(&(pSslContext->privKey), (const unsigned char *)sess->sslCert->pPrivateKey,
                                                sess->sslCert->privateKeySize, NULL, 0, mbedtls_ctr_drbg_random,
                                                &(pSslContext->ctrDrbgContext));
        }
        if (mbedtlsError != 0) {
            htdbgprintf("%s:%d: set priv key fail.\n", __func__, __LINE__);
            return -1;
        }

        if (sess->sslCert->pClientCert) {
            mbedtlsError =
                mbedtls_ssl_conf_own_cert(&(pSslContext->config), &(pSslContext->clientCert), &(pSslContext->privKey));
        }
        if (mbedtlsError != 0) {
            htdbgprintf("%s:%d: set mutual auth fail.\n", __func__, __LINE__);
            return -1;
        }
    }

    return 0;
}

void mbedtls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    const char *p, *basename;
    (void)ctx;

    /* Extract basename from file */
    for (p = basename = file; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }

    htdbgprintf("%s:%04d: |%d| %s", basename, line, level, str);
}
static int sslSetup(SSLContext_t *pSslContext)
{
    int32_t mbedtlsError = -1;

    mbedtlsError = mbedtls_ssl_config_defaults(&(pSslContext->config), MBEDTLS_SSL_IS_CLIENT,
                                               MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (mbedtlsError != 0) {
        return -1;
    }
    // mbedtlsError = mbedtls_ssl_set_hostname( &( pSslContext->context ), (const char *)sess->hcs_host);
    if (mbedtlsError != 0) {
        return -1;
    }

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_ssl_conf_dbg(&(pSslContext->config), mbedtls_debug, stdout);
    mbedtls_debug_set_threshold(0);  // set to be 4 for mbedtls debug
#endif

    return 0;
}

static int sslHandshake(httpclient_sess *sess, SSLContext_t *pSslContext)
{
    int32_t mbedtlsError = -1;
    mbedtlsError = mbedtls_ssl_setup_pre_allocate(&(pSslContext->context), &(pSslContext->config));
#if 0
    //test begin
    heap_status hs={0};
    if(qapi_Heap_Status(&hs) != QAPI_OK)
    {
        printf("test ask qapi_Heap_Status fail.\n");
    }
    else{
        printf("test heap INFO:%d,%d,%d,%d \n",hs.total_Bytes, hs.total_Bytes-hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes);
    }
    //test end
#endif
    if (mbedtlsError != 0) {
        htdbgprintf("%s:%d: mbedtls_ssl_setup fail.\n", __func__, __LINE__);
        return mbedtlsError;
    }

    mbedtls_ssl_set_bio(&(pSslContext->context), (void *)&(sess->hcs_socket), mbedtls_net_send, mbedtls_net_recv, NULL);

    do {
        mbedtlsError = mbedtls_ssl_handshake(&(pSslContext->context));
    } while ((mbedtlsError == MBEDTLS_ERR_SSL_WANT_READ) || (mbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE));

    if (mbedtlsError != 0) {
        htdbgprintf("%s:%d: mbedtls_ssl_handshake fail.\n", __func__, __LINE__);
        return mbedtlsError;
    }

    mbedtlsError = mbedtls_ssl_get_verify_result(&(pSslContext->context));
    if (mbedtlsError != 0) {
        htdbgprintf("%s:%d: mbedtls_ssl_get_verify_result fail.\n", __func__, __LINE__);
        return mbedtlsError;
    }

    return 0;
}

/* FUNCTION: http_client_sslconnect()
 *
 * New ssl con and connect to ssl server
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */

static int http_client_sslconnect(httpclient_sess *sess)
{
    int ret = 0;

    htdbgprintf("http_client_sslconnect.\n");

    if (!sess->sslCtx) {
        sess->sslCtx = (SSLContext_t *)mbedtls_calloc(1, sizeof(SSLContext_t));
    }
    if (!sess->sslCtx) {
        htdbgprintf("http_client_sslconnect fail, ctx alloc fail.\n");
        return -1;
    }

    ret = sslContextInit(sess->sslCtx);

    if (ret != 0) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
        htdbgprintf("http_client_sslconnect fail, ctx init fail.\n");
        return -1;
    }

    ret = sslSetup(sess->sslCtx);
    if (ret != 0) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
        htdbgprintf("http_client_sslconnect fail, ssl setup fail.\n");
        return -1;
    }

    ret = sslSetCredentials(sess, sess->sslCtx);
    if (ret != 0) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
        htdbgprintf("http_client_sslconnect fail, set credential fail.\n");
        return -1;
    }
    // set pre-allocte flag
    pre_allocte_big_memory = sess->is_pre_alccote_ssl_buffer;

    ret = sslHandshake(sess, sess->sslCtx);
    if (ret != 0) {
        sslContextFree(sess->sslCtx);
        sess->sslCtx = NULL;
        htdbgprintf("http_client_sslconnect fail, ssl handshake fail, ret:%x.\n", ret);
        return -1;
    }

    return 0;
}

/* FUNCTION: http_client_sslconfigure()
 *
 * Configure SSL connection
 *
 * PARAMS: sess
 * PARAMS: cfg
 *
 * RETURNS: OK or ERROR code
 */

int http_client_sslconfigure(httpclient_sess *sess, qapi_Ssl_Config_t *cfg)
{
    htdbgprintf("http_client_sslconfigure\n");
    if (http_client_sess_is_found(sess) != HTTPC_OK || cfg == NULL) {
        return HTTPC_ERROR;
    }

    sess->sslCfg = cfg;

    return HTTPC_OK;
}

/* FUNCTION: http_client_free_sslconfigure()
 *
 * Free SSL Configuration
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_free_sslconfigure(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (sess == NULL || (sess->sslCfg == NULL))
        return HTTPC_ERR_SSL_NONE_CONF;

    if (sess->sslCfg->server_name) {
        free(sess->sslCfg->server_name);
        sess->sslCfg->server_name = NULL;
    }

    if (sess->sslCfg->alpn_string) {
        free(sess->sslCfg->alpn_string);
        sess->sslCfg->alpn_string = NULL;
    }

    free(sess->sslCfg);
    sess->sslCfg = NULL;

    return HTTPC_OK;
}

/* FUNCTION: return HTTPC_OK;()
 *
 * Configure SSL cert
 *
 * PARAMS: sess
 * PARAMS: cert
 *
 * RETURNS: OK or ERROR code
 */

int http_client_certconfigure(httpclient_sess *sess, qapi_Ssl_Cert_t *crt)
{
    if (http_client_sess_is_found(sess) != HTTPC_OK || crt == NULL) {
        return HTTPC_ERROR;
    }

    sess->sslCert = crt;

    return HTTPC_OK;
}

/* FUNCTION: http_client_free_sslcert()
 *
 * Free SSL Certificate
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
int http_client_free_sslcert(httpclient_sess *sess)
{
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (sess == NULL || (sess->sslCert == NULL))
        return HTTPC_ERR_SSL_NONE_CONF;

    if (sess->sslCert->pRootCa) {
        free(sess->sslCert->pRootCa);
        sess->sslCert->pRootCa = NULL;
    }

    if (sess->sslCert->pClientCert) {
        free(sess->sslCert->pClientCert);
        sess->sslCert->pClientCert = NULL;
    }

    if (sess->sslCert->pPrivateKey) {
        free(sess->sslCert->pPrivateKey);
        sess->sslCert->pPrivateKey = NULL;
    }

    free(sess->sslCert);
    sess->sslCert = NULL;

    return HTTPC_OK;
}

/* FUNCTION: http_client_sendpkt()
 *
 * send packet via socket API
 *
 * PARAMS: sess, socket, buffer, length
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_sendpkt(httpclient_sess *sess, int32_t socket, uint8_t *buffer, int length)
{
    uint32_t buffer_offset = 0;
    uint32_t bytes_to_send = length;
    uint32_t bytes_sent = 0;

    while (1) {
        bytes_sent = 0;

        if (sess->sslCtx) {
            bytes_sent = mbedtls_ssl_write(&(sess->sslCtx->context), &buffer[buffer_offset], bytes_to_send);
        } else {
            bytes_sent = send(socket, (char *)&buffer[buffer_offset], bytes_to_send, 0);
            htdbgprintf("http_client_sendpkt, bytes_sent=%d\n", bytes_sent);
        }

        if (bytes_sent > 0) {
            if (bytes_sent == bytes_to_send)
                return HTTPC_OK;

            bytes_to_send -= bytes_sent;
            buffer_offset += bytes_sent;
            qurt_thread_sleep(1);
        } else {
            // severe push back, let the other processes run for 1 systick
            qurt_thread_sleep(1);
        }
    }
}

int http_client_send_chunk(httpclient_sess *sess, HTTPC_REQUEST_CMD_E cmd, const char *url, const char *chunk_data,
                           int32_t chunk_size, uint8_t chunk_flag, int32_t total_size)
{
    char *pktbuf;
    uint32_t pkt_len = 0;
    int32_t offset;
    char v6addr[16];
    int32_t error = HTTPC_OK;
    rx_cb_t *cb;

    /*
     * if http client session is closed by http server,
     * http client will do reconnection in next data sending.
     * here we just check if http client is in HCS_ENDING state
     * to avoid return error without reconnection.
     */
    if (sess && sess->hcs_state == HCS_ENDING) {
        return HTTPC_ERR_BUSY;
    }

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK) {
        htdbgprintf("%s %d fatal error!\n", __func__, __LINE__);
        return error;
    }

    cb = &(sess->cb);
    cb->pending_req = TRUE;
    sess->hcs_lasttime = HTTPCTICKS;

    /* Error: Have NOT got the response for previous HTTP request. */
    if (cb->pending_req) {
        // return HTTPC_ERR_BUSY;
    }

    /* Reconnect to HTTP server */
    if (HTTPC_FLAG_IS_SET(sess, HCF_CLOSED_BY_PEER) && (sess->hcs_state == HCS_INIT)) {
        http_client_connect(sess, (const char *)sess->hcs_host, sess->hcs_port);
    }

    if (sess->hcs_state != HCS_CONNECTED) {
        htdbgprintf("error status %d.\n", sess->hcs_state);
        if (sess->hcs_state < HCS_CONNECTED)
            return HTTPC_ERR_CONN;
        else
            return HTTPC_ERR_BUSY;
    }

    HTTPC_RESET_FLAG(sess, HCF_CLOSED_BY_PEER);

    /* HTTP Body for POST/PUT/PATCH command */
    if ((cmd != HTTP_CLIENT_POST_CMD && cmd != HTTP_CLIENT_PUT_CMD && cmd != HTTP_CLIENT_PATCH_CMD)) {
        return HTTPC_ERROR;
    }

    sess->remain_len = 0;
    HTTPC_RESET_FLAG(sess, HCF_RESP_CONT);

    /* Copy the page name URI and make it an absolute path */
    if (!url) {
        sess->hcs_url[0] = '/';
    } else if (*url != '/') {
        sess->hcs_url[0] = '/';
        strlcpy((char *)(sess->hcs_url + 1), url, sizeof(sess->hcs_url) - 1);
    } else
        strlcpy((char *)(sess->hcs_url), url, sizeof(sess->hcs_url));

    sess->hcs_command = cmd;

    if (chunk_flag & HTTPC_WITH_HEADER_MASK) {
        pkt_len += strlen(http_req[sess->hcs_command]) + strlen((char *)sess->hcs_url) +
                   12; /*for "POST /path/index.php HTTP/1.1\r\n" */
        pkt_len += 26; /* for "Connection: keep-alive\r\n\r\n" */
        if (sess->hcs_host_flag == 0) {
            pkt_len += strlen((char *)sess->hcs_host) + 16; /* for "Host: <host or url>:nnnnn\r\n" */
        }

        if (sess->hcs_headerbufoffset > 0) /* user has called add_header_field() to specify its own headers */
        {
            pkt_len += sess->hcs_headerbufoffset;
        } else {
            pkt_len += 97;  // for default headers:
                            // "Content-Type: text/plain\r\n"
                            // "Accept: text/html, */*\r\n"
                            // "Cache-control: no-cache\r\n"
                            // "User-Agent: IOE Client\r\n"
        }

        if (chunk_flag & HTTPC_CHUNKED_MASK) {
            pkt_len += 52;  // for chunked encoding headers(with above fileds):
                            // "Expect: 100-continue\r\n"
                            // "Transfer-Encoding: chunked\r\n"
        }

        if (sess->hcs_command == HTTP_CLIENT_PUT_CMD) {
            pkt_len += 24;  // "Content-length: nnnnnn\r\n"
        }
    }

    if (chunk_flag & HTTPC_CHUNKED_MASK) {
        pkt_len += sizeof(long) + chunk_size + 4;  //[chunk size][\r\n][chunk data][\r\n]
    } else {
        if (chunk_size != 0)
            pkt_len += chunk_size;
        else
            pkt_len += sizeof(long);  //'0' end for raw http packet
    }
    if ((pktbuf = malloc(pkt_len)) == NULL) {
        htdbgprintf("%s %d no buffer\n", __func__, __LINE__);
        return HTTPC_ERR_NO_MEMORY;
    }
    memset(pktbuf, 0, pkt_len);

    offset = 0;

    /*need add header info in first chunk/raw packet*/
    if (chunk_flag & HTTPC_WITH_HEADER_MASK) {
        /* START line:
         * "POST /path/index.php HTTP/1.1\r\n"
         */
        offset += snprintf(pktbuf + offset, pkt_len - offset, "%s %s", http_req[sess->hcs_command], sess->hcs_url);
        offset += snprintf(pktbuf + offset, pkt_len - offset, " HTTP/1.1\r\n");

        if (sess->hcs_host_flag == 0) {
            /* Host header:
             * "Host: www.example.com:8080\r\n"     -or-
             * "Host: www.example.com\r\n"          -or-
             * "Host: [fdda:5cc1:23:4::1f]\r\n"     -or-
             * "Host: [fdda:5cc1:23:4::1f]:4433\r\n"
             */
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Host: ");
            if (inet_pton(AF_INET6, (char *)sess->hcs_host, v6addr) == 1) /* hcs_host[] is an IPv6 address string */
            {
                offset += snprintf(pktbuf + offset, pkt_len - offset, "[%s]", sess->hcs_host);
            } else {
                offset += snprintf(pktbuf + offset, pkt_len - offset, "%s", sess->hcs_host);
            }

            /* Append ":<port>" if port is not the default HTTP/HTTPS listening port */
            if (sess->hcs_port != HTTP_DEFAULT_PORT && sess->hcs_port != HTTPS_DEFAULT_PORT) {
                offset += snprintf(pktbuf + offset, pkt_len - offset, ":%u", sess->hcs_port);
            }
            offset += snprintf(pktbuf + offset, pkt_len - offset, "\r\n");
        }

        /* User-specified headers or default headers */
        if (sess->hcs_headerbufoffset > 0) /* user has called add_header_field() to specify its own headers */
        {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "%s", sess->hcs_headerbuffer);
        } else {
            if (sess->hcs_command == HTTP_CLIENT_POST_CMD) {
                offset += snprintf(pktbuf + offset, pkt_len - offset, "Content-Type: text/plain\r\n");
            }
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Accept: text/html, */*\r\n");
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Cache-control: no-cache\r\n");
            offset += snprintf(pktbuf + offset, pkt_len - offset, "User-Agent: IOE Client\r\n");
        }

        if (chunk_flag & HTTPC_CHUNKED_MASK) {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Expect: 100-continue\r\n");
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Transfer-Encoding: chunked\r\n");
        }

        if (sess->hcs_command == HTTP_CLIENT_PUT_CMD) {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "Content-length: %lu\r\n", total_size);
        }

        /* Connection header */
        offset += snprintf(pktbuf + offset, pkt_len - offset, "Connection: keep-alive\r\n");

        /* a blank line to indicate the end of HEADER section */
        offset += snprintf(pktbuf + offset, pkt_len - offset, "\r\n");
    }

    if (chunk_flag & HTTPC_CHUNKED_MASK) {
        if (chunk_size > 0) {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "%lx\r\n", chunk_size);
            /* HTTP Body for POST/PUT/PATCH command */

            if (sess->hcs_bufoffset > 0 &&
                (sess->hcs_command == HTTP_CLIENT_POST_CMD || sess->hcs_command == HTTP_CLIENT_PUT_CMD)) {
                memcpy(pktbuf + offset, sess->hcs_buffer, sess->hcs_bufoffset);
                offset += sess->hcs_bufoffset;
            } else if (chunk_data) {
                memcpy(pktbuf + offset, chunk_data, chunk_size);
                offset += chunk_size;
            } else {
                free(pktbuf);
                htdbgprintf("%s %d invalid data\n", __func__, __LINE__);
                return HTTPC_ERR_INVALID_PARAM;
            }

        } else {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "%lx\r\n", chunk_size);
        }

        offset += snprintf(pktbuf + offset, pkt_len - offset, "\r\n");
    } else {
        if (chunk_size > 0) {
            memcpy(pktbuf + offset, chunk_data, chunk_size);
            offset += chunk_size;
        } else {
            offset += snprintf(pktbuf + offset, pkt_len - offset, "%lx", chunk_size);
        }
    }

    /* Send it */
    error = http_client_sendpkt(sess, sess->hcs_socket, (uint8_t *)pktbuf, offset);

    free(pktbuf);
    return error;
}

/* FUNCTION: http_client_sendreq()
 *
 * process packet content and send it via socket API
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_sendreq(httpclient_sess *sess)
{
    char *hdrbuf;
    uint32_t pkt_len;
    uint32_t hdr_len = 0;
    int32_t offset;
    int32_t error = HTTPC_OK;
    rx_cb_t *cb;
    char v6addr[16];

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK) {
        htdbgprintf("%s %d fatal error!\n", __func__, __LINE__);
        return error;
    }

    sess->remain_len = 0;
    HTTPC_RESET_FLAG(sess, HCF_RESP_CONT);

    htdbgprintf("%s() State:%u Flags:0x%x, index = %d\n", __func__, sess->hcs_state, sess->hcs_flags, sess->index);
    htdbgprintf("%s() URL:%s Command:%u\n", __func__, sess->hcs_url, sess->hcs_command);

    /* Allocate a buffer for sending an HTTP request.
     * The size of buffer is large enough so that a
     * check for buffer overflow is not needed.
     */
    if (sess->hcs_host_flag == 0) {
        hdr_len = max(strlen((char *)sess->hcs_host), strlen((char *)sess->hcs_url)) +
                  16; /* for "Host: <host or url>:nnnnn\r\n" */
    }
    hdr_len += 24;                     /* for "Connection: keep-alive\r\n" */
    hdr_len += 24;                     /* for "Content-length: nnnnnn\r\n" */
    if (sess->hcs_headerbufoffset > 0) /* user has called add_header_field() to specify its own headers */
    {
        hdr_len += sess->hcs_headerbufoffset;
    } else {
        hdr_len += 124;  // for default headers:
                         // "Content-Type: application/x-www-form-urlencoded\r\n"
                         // "Accept: text/html, */*\r\n"
                         // "Cache-control: no-cache\r\n"
                         // "User-Agent: IOE Client\r\n"
    }
    pkt_len = strlen((char *)sess->hcs_url) + 20 +     /* START line: "CONNECT <requri> HTTP/1.1\r\n" */
              hdr_len +                                /* header section */
              sess->hcs_bufoffset +                    /* user-specified data:
                                                          either name/value strings for GET/POST
                                                          or body message */
              HTTPCLIENT_DEFAULT_REMAIN_BUFFER_LENGTH; /* extra */

    if ((hdrbuf = malloc(pkt_len)) == NULL) {
        htdbgprintf("%s %d no buffer\n", __func__, __LINE__);
        return HTTPC_ERR_NO_MEMORY;
    }
    memset(hdrbuf, 0, pkt_len);

    cb = &(sess->cb);
    cb->pending_req = TRUE;
    sess->hcs_lasttime = HTTPCTICKS;
    offset = 0;
    htdbgprintf("%s Connected socket, cb->pending_req = %d\n", __func__, cb->pending_req);

    /* START line:
     * "GET /path/index.html HTTP/1.1\r\n"  -or-
     * "CONNECT www.example.com:80 HTTP/1.1\r\n"
     */
    /* "GET /path/script.cgi" */
    offset += snprintf(hdrbuf + offset, pkt_len - offset, "%s %s", http_req[sess->hcs_command], sess->hcs_url);
    if (sess->hcs_command == HTTP_CLIENT_GET_CMD && sess->hcs_bufoffset > 0) {
        /* "GET /path/script.cgi?name1=value1&name2=value2" */
        offset += snprintf(hdrbuf + offset, pkt_len - offset, "?%s", sess->hcs_buffer);

        memset(sess->hcs_buffer, 0, sess->hcs_bufoffset);
        sess->hcs_bufoffset = 0;
    }
    offset += snprintf(hdrbuf + offset, pkt_len - offset, " HTTP/1.1\r\n");

    if (sess->hcs_host_flag == 0) {
        /* Host header:
         * "Host: www.example.com:8080\r\n"     -or-
         * "Host: www.example.com\r\n"          -or-
         * "Host: [fdda:5cc1:23:4::1f]\r\n"     -or-
         * "Host: [fdda:5cc1:23:4::1f]:4433\r\n"
         */
        offset += snprintf(hdrbuf + offset, pkt_len - offset, "Host: ");
        if (sess->hcs_command == HTTP_CLIENT_CONNECT_CMD) {
            offset += snprintf(hdrbuf + offset, pkt_len - offset, "%s\r\n", sess->hcs_url);
        } else {
            if (inet_pton(AF_INET6, (char *)sess->hcs_host, v6addr) == 1) /* hcs_host[] is an IPv6 address string */
            {
                offset += snprintf(hdrbuf + offset, pkt_len - offset, "[%s]", sess->hcs_host);
            } else {
                offset += snprintf(hdrbuf + offset, pkt_len - offset, "%s", sess->hcs_host);
            }

            /* Append ":<port>" if port is not the default HTTP/HTTPS listening port */
            if (sess->hcs_port != HTTP_DEFAULT_PORT && sess->hcs_port != HTTPS_DEFAULT_PORT) {
                offset += snprintf(hdrbuf + offset, pkt_len - offset, ":%u", sess->hcs_port);
            }
            offset += snprintf(hdrbuf + offset, pkt_len - offset, "\r\n");
        }
    }
    /* Content-length and/or Content-type */
    if (sess->hcs_bufoffset > 0 &&
        (sess->hcs_command == HTTP_CLIENT_POST_CMD || sess->hcs_command == HTTP_CLIENT_PUT_CMD ||
         sess->hcs_command == HTTP_CLIENT_PATCH_CMD)) {
        offset += snprintf(hdrbuf + offset, pkt_len - offset, "Content-length: %lu\r\n", sess->hcs_bufoffset);

        if (sess->hcs_headerbufoffset == 0 && /* user did not specify his own headers */
            sess->hcs_command == HTTP_CLIENT_POST_CMD) {
            offset +=
                snprintf(hdrbuf + offset, pkt_len - offset, "Content-Type: application/x-www-form-urlencoded\r\n");
        }
    }

    /* User-specified headers or default headers */
    if (sess->hcs_headerbufoffset > 0) /* user has called add_header_field() to specify its own headers */
    {
        offset += snprintf(hdrbuf + offset, pkt_len - offset, "%s", sess->hcs_headerbuffer);
    } else {
        if (sess->hcs_command != HTTP_CLIENT_CONNECT_CMD) {
            offset += snprintf(hdrbuf + offset, pkt_len - offset, "Accept: text/html, */*\r\n");
            offset += snprintf(hdrbuf + offset, pkt_len - offset, "Cache-control: no-cache\r\n");
        }

        offset += snprintf(hdrbuf + offset, pkt_len - offset, "User-Agent: IOE Client\r\n");
    }

    /* Connection header */
    offset += snprintf(hdrbuf + offset, pkt_len - offset, "Connection: keep-alive\r\n");

    /* a blank line to indicate the end of HEADER section */
    offset += snprintf(hdrbuf + offset, pkt_len - offset, "\r\n");

    /* HTTP Body for POST/PUT/PATCH command */
    if (sess->hcs_bufoffset > 0 &&
        (sess->hcs_command == HTTP_CLIENT_POST_CMD || sess->hcs_command == HTTP_CLIENT_PUT_CMD ||
         sess->hcs_command == HTTP_CLIENT_PATCH_CMD)) {
        memcpy(hdrbuf + offset, sess->hcs_buffer, sess->hcs_bufoffset);
        offset += sess->hcs_bufoffset;
    }

    /* Send it */
    error = http_client_sendpkt(sess, sess->hcs_socket, (uint8_t *)hdrbuf, offset);

    free(hdrbuf);

    /* Make it zero so that the next req will be clean */
    memset(sess->hcs_buffer, 0, sess->hcs_buf_len);
    sess->hcs_bufoffset = 0;

    return error;
}

/* FUNCTION: http_client_senddata()
 *
 * send raw data via socket
 *
 * RETURNS: OK or ERROR code
 */
int http_client_senddata(httpclient_sess *sess, char *buf, int length)
{
    int error;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK) {
        return error;
    }

    if (HTTPC_FLAG_IS_RESET(sess, HCF_TUNNEL_ESTABLISHED)) {
        return HTTPC_ERROR;
    }

    return http_client_sendpkt(sess, sess->hcs_socket, (uint8_t *)buf, length);
}

/* FUNCTION: http_client_timer()
 *
 * 500ms timer to process timeout
 *
 * PARAMS: void
 *
 * RETURNS:
 */
static void http_client_timer(void)
{
    uint32_t index;
    httpclient_sess *sess;
    uint32_t timeout;
    rx_cb_t *cb;

    if (!g_httpc_ctxt)
        return;

    /* If there is no http client session, just return */
    for (index = 0; index < httpc_max_num_con; index++) {
        qurt_mutex_lock(g_httpc_ctxt->lh);
        sess = (httpclient_sess *)g_httpc_ctxt->httpc_sess[index];

        if (!sess || (sess->timeout == HTTPC_TIMEOUT_DISABLE)) {
            qurt_mutex_unlock(g_httpc_ctxt->lh);
            continue;
        }

        timeout = (sess->timeout * TICKS_PER_SEC) / 1000;
        if (timeout == 0) {
            timeout = 1; /* 1 system tick */
        }
        cb = &(sess->cb);
        if (cb->pending_req && TIME_EXP(TIME_ADD(sess->hcs_lasttime, timeout), HTTPCTICKS) && sess->hcs_lasttime) {
            htdbgprintf("Timer lasttick=%d cticks=%d\n", sess->hcs_lasttime, HTTPCTICKS);
            htdbgprintf("hcs_state = %d, flags = 0x%x, socket = %d\n", sess->hcs_state, sess->hcs_flags,
                        sess->hcs_socket);

            /* HTTP request times out */
            http_client_rx_cb(sess, HTTPC_RX_ERROR_CLIENT_TIMEOUT, NULL, 0);
            cb->pending_req = FALSE;
            http_client_disconnect(sess);
        }

        qurt_mutex_unlock(g_httpc_ctxt->lh);
    }
    return;
}

void ota_http_client_timer(void)
{
    uint32_t index;
    httpclient_sess *sess;
    uint32_t timeout;
    rx_cb_t *cb;

    if (!g_httpc_ctxt)
        return;

    /* If there is no http client session, just return */
    for (index = 0; index < httpc_max_num_con; index++) {
        qurt_mutex_lock(g_httpc_ctxt->lh);
        sess = (httpclient_sess *)g_httpc_ctxt->httpc_sess[index];

        cb = &(sess->cb);

        cb->pending_req = FALSE;
        g_httpc_ctxt->timer_lasttime = HTTPCTICKS;

        qurt_mutex_unlock(g_httpc_ctxt->lh);
    }
    return;
}

/* FUNCTION: http_client_rx_cb()
 *
 * process callback funtion which define by user
 *
 * PARAMS: sess, state, chunk_data, chunk_size
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_rx_cb(httpclient_sess *sess, int32_t state, void *chunk_data, int32_t chunk_size)
{
    http_client_cb_resp_t cb_resp;
    rx_cb_t *cb;
    int32_t error = HTTPC_OK;

    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    cb = &(sess->cb);
    cb_resp.length = chunk_size;
    cb_resp.resp_code = cb->resp_code;
    cb_resp.data = NULL;
    cb_resp.contentlength = sess->hcs_contentlength;

    if (chunk_data && chunk_size) {
        cb_resp.data = chunk_data;
    }

    if (sess->http_client_cb)
        sess->http_client_cb(sess->cb_arg, state, &cb_resp);

    return HTTPC_OK;
}

int http_client_cb_eable_adding_header(httpclient_sess *sess, int32_t enable)
{
    int32_t error = HTTPC_OK;

    /*first check whether the seession is in recording */
    if ((error = http_client_sess_is_found(sess)) != HTTPC_OK)
        return error;

    if (!!enable)
        HTTPC_SET_FLAG(sess, HCF_CB_ADD_HEAD);
    else
        HTTPC_RESET_FLAG(sess, HCF_CB_ADD_HEAD);

    return HTTPC_OK;
}

int https_client_recv_handle(httpclient_sess *sess, uint8_t *buf, int length)
{
    int readval, err, state;
    int rx_len = 0;

    do {
        readval = mbedtls_ssl_read(&(sess->sslCtx->context), buf, length);

        if (readval == MBEDTLS_ERR_SSL_WANT_READ || readval == MBEDTLS_ERR_SSL_WANT_WRITE ||
            readval == MBEDTLS_ERR_SSL_TIMEOUT) {
            continue;
        }

        if (readval <= 0) {
            int len = (int)sizeof(int);

            getsockopt(sess->hcs_socket, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
            htdbgprintf("%s: readval:%d ERRNO:%d\n", __func__, readval, err);
            if ((err == EWOULDBLOCK) || (err == ENOBUFS)) {
                readval = 0; /* No new data arrived */
            } else {
                /* Server has closed the connection */
                if (err == 0 ||                        /* for IPv6 */
                    err == ECONNRESET || err == EPIPE) /* for IPv4 */
                {
                    HTTPC_SET_FLAG(sess, HCF_CLOSED_BY_PEER);

                    if (HTTPC_FLAG_IS_SET(sess, HCF_TUNNEL_ESTABLISHED)) {
                        state = HTTPC_RX_TUNNEL_CLOSED;
                        HTTPC_RESET_FLAG(sess, HCF_TUNNEL_ESTABLISHED);
                    } else {
                        state = HTTPC_RX_ERROR_SERVER_CLOSED;
                    }
                } else /* Treat other errno as internal read error */
                {
                    state = HTTPC_RX_ERROR_CONNECTION_CLOSED;
                }

                /* Let the app know the state */
                http_client_rx_cb(sess, state, NULL, 0);

                http_client_disconnect(sess);
            }

            break;
        } else {
            sess->hcs_lasttime = HTTPCTICKS; /* reset activity timer */
            sess->hcs_rxbufoffset += readval;
            *(sess->hcs_rxbuffer + sess->hcs_rxbufoffset) = '\0';
            err = http_client_processpkt(sess, readval);
#ifdef CONFIG_QAT_OTA_DEMO
            if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
                /*no need to print when running OTA over HTTP*/
            } else
#endif
            {
                htdbgprintf("%s:%d readval%d err:%d\n", __func__, __LINE__, readval, err);
            }

            rx_len += readval;
        }
        if (mbedtls_ssl_get_bytes_avail(&(sess->sslCtx->context)) == 0) {
            break;
        }

    } while (1);

    return rx_len;
}

/* FUNCTION: http_client_readsock()
 *
 * get buffer by reading socket
 *
 * PARAMS: sess
 *
 * RETURNS: readval
 */
static int http_client_readsock(httpclient_sess *sess)
{
    int readval, length;
    char *buf;

    buf = (char *)(sess->hcs_rxbuffer + sess->hcs_rxbufoffset);
    /* Save one byte for NULL termination to permit string operations: stristr(), etc. */
    length = (sess->hcs_rxbuf_len - 1) - sess->hcs_rxbufoffset;

    if (sess->sslCtx) {
        return https_client_recv_handle(sess, (unsigned char *)buf, length);
    } else {
        readval = recv(sess->hcs_socket, buf, length, 0);
    }
    return readval;
}

static void jumpOverSpaces(char **strP)
{
    if (strP == NULL || *strP == NULL)
        return;

    while (**strP != '\0') {
        if (**strP == ' ') {
            (*strP)++;
            continue;
        }
        break;
    }
    return;
}

static uint32_t get_http_response_code(char *str)
{
    uint32_t rspCode = 0;
    char *at_stop_p = NULL;

    if (str == NULL)
        return 0;

    jumpOverSpaces(&str);

    // setlocale(LC_ALL,"C");
    rspCode = (uint32_t)strtoul(str, &at_stop_p, 10);

    return rspCode;
}

static boolean isAllSpaces(char *str, int Length)
{
    int i;

    if (str == NULL)
        return FALSE;

    for (i = 0; str[i] != '\0' && i < Length; i++) {
        if (str[i] == ' ')
            continue;

        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Parses HTTP header information
 *
 * @details HTTP header information is parsed to look for the specific field as
 *          requested by the caller and the value associated with the field is
 *          returned to the caller.
 *
 * @param[in]  linetype   HTTP header field of interest
 * @param[in]  httphdr    HTTP header buffer
 * @param[in]  len        HTTP header buffer size
 * @param[out] hdr_val    HTTP header field value of interest
 *
 *
 * @return HTTPCLIENT_RSP_HEADER_STATUS, value associated with the response
 *         header status: no header, with header of invalid or valid format.
 */
static http_client_rsp_header_status http_client_get_header_value(char *linetype, char *httphdr, int len, void *hdr_val)
{
    http_client_rsp_header_status ret_status;
    char *cp = NULL;
    char *hdr_val_p = NULL;
    int typelen = 0;
    int val_len = 0;

    if (linetype == NULL || httphdr == NULL || len == 0) {
        ret_status = HTTP_CLIENT_RSP_NO_HEADER;
        goto exit;
    }

    typelen = strlen(linetype);
    ret_status = HTTP_CLIENT_RSP_NO_HEADER;

    /*
     * Find header in format:
     * "\r\nheader-name:<OWS>header_value<OWS>\r\n"
     * where OWS means optional whitespace, none or single or multiple.
     */
    for (cp = httphdr + 2; cp < (httphdr + len); cp++) {
        if ((*cp | 0x20) == (*linetype | 0x20)) /* Got a match for first char */
        {
            if (strncasecmp(cp, linetype, typelen) == 0) {
                if (strncmp(cp - 2, "\r\n", 2) != 0) {
                    ret_status = HTTP_CLIENT_RSP_WITH_INVALID_FORMAT_HEADER;
                    goto exit;
                } else {
                    ret_status = HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER;
                    cp += typelen;
                    jumpOverSpaces(&cp);
                    hdr_val_p = cp;
                    /* find next "\r\n"*/
                    while (cp < (httphdr + len) && strncmp(cp, "\r\n", 2) != 0) {
                        cp++;
                        val_len++;
                    }
                    break;
                }
            }
        }
        if (strncmp(cp, "\r\n\r\n", 4) == 0)
            goto exit;
    }

    if (ret_status != HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER || cp >= (httphdr + len))
        goto exit;

    if (strncasecmp(linetype, "Content-Length:", typelen) == 0) {
        int32_t hcs_contentlen = 0;
        int32_t *hcs_contentlen_p = (int32_t *)hdr_val;
        char *at_stop_p = NULL;

        hcs_contentlen = strtoul(hdr_val_p, &at_stop_p, 10);

        if (isAllSpaces(at_stop_p, val_len - (at_stop_p - hdr_val_p))) {
            if (hcs_contentlen_p != NULL) {
                *hcs_contentlen_p = hcs_contentlen;
            }
        } else {
            *hcs_contentlen_p = (-1);
        }
        goto exit;
    }

    if (strncasecmp(linetype, "Transfer-Encoding:", typelen) == 0) {
        boolean *is_chunked_p = (boolean *)hdr_val;
        if (val_len >= 7 && (strncasecmp(hdr_val_p, "chunked", 7) == 0) && isAllSpaces(hdr_val_p + 7, val_len - 7)) {
            if (is_chunked_p != NULL) {
                *is_chunked_p = TRUE;
            }
        } else {
            *is_chunked_p = FALSE;
        }
        goto exit;
    }
exit:
    return ret_status;
}

/* FUNCTION: http_client_processpkt()
 *
 * process rx packet from socket
 *
 * PARAMS: sess, length
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_processpkt(httpclient_sess *sess, int length)
{
    rx_cb_t *cb;
    qbool_t reset_sess;
    qbool_t sendFailEvent;
    char *rxend;
    uint32_t len;

    if (!sess) {
        htdbgprintf("%s %d fatal error!\n", __func__, __LINE__);
        return HTTPC_ERROR;
    }
#ifdef CONFIG_QAT_OTA_DEMO
    if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
        /*no need to print when running OTA over HTTP*/
    } else
#endif
    {
        htdbgprintf("Received packet. Size:%u.BufferOff:%u\n", length, sess->hcs_rxbufoffset);
    }

    if (!length) {
        htdbgprintf("length = 0\n");
        return HTTPC_ERR_INVALID_PARAM;
    }

    /* If the HTTP request has not been sent, don't process the remaining packet */
    cb = &sess->cb;
    if (!cb->pending_req) /* No pending HTTP request */
    {
        if (HTTPC_FLAG_IS_SET(sess, HCF_TUNNEL_ESTABLISHED)) {
            /* Return non-HTTP data to user */
            cb->state = HTTPC_RX_DATA_FROM_TUNNEL;
            http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer, length);
            sess->hcs_rxbufoffset = 0;
            return HTTPC_OK;
        }
        return HTTPC_ERROR;
    }

    rxend = NULL;
    reset_sess = FALSE;
    sendFailEvent = TRUE;

    if (HTTPC_FLAG_IS_RESET(sess, HCF_RESP_CONT)) {
        /* sample response:
         * HTTP/1.1 200 OK\r\n
         * Date: Fri, 31 Dec 2000 23:59:59 GMT\r\n
         * Server: Webserver 1.0\r\n
         * Content-type: text/html\r\n
         * Transfer-Encoding: gzip, chunked\r\n
         * \r\n
         * ...
         */
        /* We have not read all headers into rxbuffer yet */
        if (HTTPC_FLAG_IS_RESET(sess, HCF_RECV_HEADER)) {
            int32_t hcs_contentlen = -1;
            boolean is_chunked = FALSE;
            http_client_rsp_header_status ret_status;

            rxend = strstr((const char *)sess->hcs_rxbuffer, "\r\n\r\n");
            if (rxend) /* Received all headers */
            {
                HTTPC_SET_FLAG(sess, HCF_RECV_HEADER);

                /* Calculate the size of header section */
                sess->hcs_headersize = rxend - (char *)sess->hcs_rxbuffer + 4; /* 4 for \r\n\r\n */

                /* Get response code */
                cb->resp_code = get_http_response_code((char *)sess->hcs_rxbuffer + 9);
                if (cb->resp_code < HTTP_RESPONSE_CODE_MIN || cb->resp_code > HTTP_RESPONSE_CODE_MAX) {
                    reset_sess = TRUE;
                    cb->state = HTTPC_RX_ERROR_INVALID_RESPONSECODE;
                    goto end;
                }
                /* for HEAD request, we return all headers and done! */
                if (sess->hcs_command == HTTP_CLIENT_HEAD_CMD) {
                    cb->state = HTTPC_RX_FINISHED;

                    /* Get length of message body if there is "Content-Length" header */
                    sess->hcs_contentlength = 0;
                    ret_status = http_client_get_header_value("Content-Length:", (char *)sess->hcs_rxbuffer,
                                                              sess->hcs_headersize, &hcs_contentlen);
                    if (ret_status >= HTTP_CLIENT_RSP_WITH_INVALID_FORMAT_HEADER) {
                        if (ret_status == HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER && hcs_contentlen >= 0) {
                            sess->hcs_contentlength = (uint32_t)hcs_contentlen;
                        } else {
                            reset_sess = 1;
                            cb->state = HTTPC_RX_ERROR_RX_PROCESS;
                            goto end;
                        }
                    }

                    http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer, sess->hcs_headersize);
                    reset_sess = TRUE;
                    sendFailEvent = FALSE;

                    goto end;
                }

                /* successful CONNECT response */
                if (sess->hcs_command == HTTP_CLIENT_CONNECT_CMD && cb->resp_code >= 200 &&
                    cb->resp_code <= 299) /* successful */
                {
                    HTTPC_SET_FLAG(sess, HCF_TUNNEL_ESTABLISHED);
                    sess->hcs_contentlength = sess->hcs_rxbufoffset - sess->hcs_headersize;
                }

                if (cb->resp_code == HTTP_RESPONSE_CODE_MIN) {
                    cb->state = HTTPC_RX_CHUNK_CONTINUE;
                    cb->pending_req = TRUE;
                    http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer, sess->hcs_headersize);
                    reset_sess = true;
                    sendFailEvent = FALSE;
                    goto end;
                }

                /* RFC 7231 4.3.6
                 * A client MUST ignore any Content-Length or Transfer-Encoding
                 * header fields received in a successful response to CONNECT.
                 */
                if (HTTPC_FLAG_IS_RESET(sess, HCF_TUNNEL_ESTABLISHED) ||
                    (sess->hcs_command != HTTP_CLIENT_CONNECT_CMD && HTTPC_FLAG_IS_SET(sess, HCF_TUNNEL_ESTABLISHED))) {
                    /* Get length of message body if there is "Content-Length" header */
                    sess->hcs_contentlength = 0;
                    ret_status = http_client_get_header_value("Content-Length:", (char *)sess->hcs_rxbuffer,
                                                              sess->hcs_headersize, &hcs_contentlen);
                    if (ret_status >= HTTP_CLIENT_RSP_WITH_INVALID_FORMAT_HEADER) {
                        if (ret_status == HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER && hcs_contentlen >= 0) {
                            sess->hcs_contentlength = (uint32_t)hcs_contentlen;
                        } else {
                            reset_sess = 1;
                            cb->state = HTTPC_RX_ERROR_RX_PROCESS;
                            goto end;
                        }
                    }

                    /* Check if this is chunked transfer-encoding */
                    ret_status = http_client_get_header_value("Transfer-Encoding:", (char *)sess->hcs_rxbuffer,
                                                              sess->hcs_headersize, &is_chunked);
                    if (ret_status >= HTTP_CLIENT_RSP_WITH_INVALID_FORMAT_HEADER) {
                        if (ret_status == HTTP_CLIENT_RSP_WITH_VALID_FORMAT_HEADER && is_chunked) {
                            HTTPC_SET_FLAG(sess, HCF_CHUNKED);
                            HTTPC_SET_FLAG(sess, HCF_RESP_CONT);
                            sess->hcs_contentlength = 0;
#ifdef HTTPC_DEBUG
                            sess->total_chunks = 0;
#endif
                        } else {
                            reset_sess = 1;
                            cb->state = HTTPC_RX_ERROR_RX_PROCESS;
                            goto end;
                        }
                    }
                }

                /* Return all headers to user via his registered callback */
                if (HTTPC_FLAG_IS_SET(sess, HCF_CB_ADD_HEAD)) {
                    cb->state = HTTPC_RX_MORE_DATA;
                    http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer, sess->hcs_headersize);
                }
            } else {
                /* Have not received all headers.
                 * There are two cases:
                 * 1. rxbuffer was full which indicates header section is too long.
                 *    We cannot handle this case so return with ERROR.
                 * 2. rxbuffer was not filled yet. So we return and hope the next
                 *    socket read will read in all headers.
                 */
                if (sess->hcs_rxbufoffset == sess->hcs_rxbuf_len - 1) {
                    reset_sess = TRUE;
                    cb->state = HTTPC_RX_ERROR_RX_HTTP_HEADER;
                }
                goto end;
            }
        } /* if (HTTPC_FLAG_IS_RESET(sess, HCF_RECV_HEADER)) */

        /* Now rxbuffer contains all headers and/or partial/whole message body */

        /* If the connection has been upgraded to another protocol after
         * processing the headers.  Call the upgrade module to process
         * any remaining data in the RX buffer */
        if (HTTPC_FLAG_IS_SET(sess, HCF_RECV_HEADER) && HTTPC_IS_UPGRADED(sess)) {
            if (sess->hcs_rxbufoffset > sess->hcs_headersize) {
                sess->protocol_upgrade_module_rx_cb(sess->protocol_upgrade_module_rx_cb_arg, sess,
                                                    (sess->hcs_rxbuffer + sess->hcs_headersize),
                                                    (sess->hcs_rxbufoffset - sess->hcs_headersize));
            }
            sess->hcs_rxbufoffset = 0;
            return HTTPC_OK;
        }

        if (HTTPC_FLAG_IS_RESET(sess, HCF_CHUNKED)) /* Not chunked transfer */
        {
            /* We received a complete HTTP response */
            if (sess->hcs_rxbufoffset >= sess->hcs_headersize + sess->hcs_contentlength) {
                /* Return message body to user via his registered callback */
                if (sess->hcs_command == HTTP_CLIENT_CONNECT_CMD && HTTPC_FLAG_IS_SET(sess, HCF_TUNNEL_ESTABLISHED)) {
                    /* RFC 7231 4.3.6
                     * Any 2xx (Successful) response indicates that the sender (and all
                     * inbound proxies) will switch to tunnel mode immediately after the
                     * blank line that concludes the successful response's header section;
                     * data received after that blank line is from the server identified by
                     * the request-target.
                     */
                    setup_tunnel(sess);
                    cb->state = HTTPC_RX_TUNNEL_ESTABLISHED;
                } else {
                    cb->state = HTTPC_RX_FINISHED;
                }
                http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer + sess->hcs_headersize, sess->hcs_contentlength);

                /* We are done! */
                reset_sess = TRUE;
                sendFailEvent = FALSE;
            }
            /* Received partial contents */
            else {
                /* Return this partial contents to user via his registered callback */
                len = sess->hcs_rxbufoffset - sess->hcs_headersize;
                cb->state = HTTPC_RX_MORE_DATA;
                http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer + sess->hcs_headersize, len);
                HTTPC_SET_FLAG(sess, HCF_RESP_CONT);
                sess->remain_len = sess->hcs_contentlength - len;
                sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
            }
            goto end;
        } /* if (HTTPC_FLAG_IS_RESET(sess, HCF_CHUNKED)) */
    }     /* if (HTTPC_FLAG_IS_RESET(sess, HCF_RESP_CONT)) */

    if (HTTPC_FLAG_IS_SET(sess, HCF_RESP_CONT)) {
        /* Not chunked transfer */
        if (HTTPC_FLAG_IS_RESET(sess, HCF_CHUNKED)) {
            if (sess->remain_len > (uint32_t)length) {
                /* We have not received all contents yet */
                cb->state = HTTPC_RX_MORE_DATA;
                sess->remain_len -= length;
                sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
            } else {
                /* Done! We received all contents. */
                cb->state = HTTPC_RX_FINISHED;
                reset_sess = TRUE;
                sendFailEvent = FALSE;
            }

            /* Return the contents to user via his registered callback */
            http_client_rx_cb(sess, cb->state, sess->hcs_rxbuffer, length);
        }

        /* chunked transfer-encoding */
        else {
            /* sample chunk:
             * 1a[;<ext_1>;<ext_2>;..<ext_n>]\r\n
             * abcdefghijklmnopqrstuvwxyz\r\n
             * 10\r\n
             * 1234567890abcdef\r\n
             * 0\r\n
             * [footer_1: value1\r\n]
             * ..
             * [footer_n: valuen\r\n]
             * \r\n
             */
            uint8_t *pkt;
            long chunk_size;
            uint8_t *chunk;      /* start of each chunk */
            uint8_t *chunk_data; /* start of chunk data */

            if (rxend) /* rxbuffer contains all headers and 0 or more chunks */
            {
                /* rxbuffer       pkt          chunk               pkt + length
                 * |               |            |                   |
                 * V               V            V                   V
                 * +-------------------------------------------------------+
                 * |               |<----------- length ----------->|      |
                 * |<------------------ rxbufoffset --------------->|      |
                 * |<------- headersize ------>|                           |
                 * |                    \r\n\r\n1a;<ext_1>;<ext_2>..       |
                 * +-------------------------------------------------------+
                 */
                pkt = sess->hcs_rxbuffer + sess->hcs_rxbufoffset - length;
                chunk = sess->hcs_rxbuffer + sess->hcs_headersize;
            } else /* rxbuffer contains only chunks */
            {
                /* set 'chunk' so 'chunk' points to either
                 * the start of a chunk     -or-
                 * the data section of a chunk
                 */
                pkt = sess->hcs_rxbuffer;
                chunk = pkt + sess->offset; /* offset: 0, 1, 2 */
                sess->offset = 0;
            }

            /* 'chunk' points to data section
             * so return data to user via his registered callback
             */
            if (sess->remain_len > 0) {
                len = min((uint32_t)length, sess->remain_len);

                cb->state = HTTPC_RX_MORE_DATA;
                http_client_rx_cb(sess, cb->state, chunk, len);

                sess->hcs_contentlength += len;
                sess->remain_len -= len;

                if (sess->remain_len != 0) {
                    sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
                    goto end;
                }

                /* locate the start of next chunk */
                chunk += len;
                chunk += 2; /* \r\n */
            }

            /* 'chunk' points to the start of the chunk */
            while (chunk < pkt + length) {
                /* Try to find the end of size section */
                chunk_data = (uint8_t *)strstr((const char *)chunk, "\r\n");

                /* If only partial size section of chunk is in rxbuffer,
                 * memmove this chunk to the beginning of rxbuffer and
                 * set rxbufoffset to the length of partial size section
                 * so that the whole chunk may be in rxbuffer after next read.
                 */
                if (chunk_data == NULL) /* didn't receive the complete size section */
                {
                    if (chunk == sess->hcs_rxbuffer) {
                        /* the size section is too long */
                        reset_sess = TRUE;
                        cb->state = HTTPC_RX_ERROR_RX_PROCESS; /* invalid chunk */
                    } else {
                        /* pkt                chunk    pkt + length
                         * |                   |        |
                         * V                   V        V
                         * +----------------------------+
                         * |<--------- length --------->|
                         * |                   |<-len ->|
                         * |                   1a;<ext_1>;<ext_2>\r\nabcdefghijklmnopqrstuvwxyz\r\n
                         * |                   |<-- size section --->|<------ data section ------>|
                         * +----------------------------+
                         *
                         */
                        len = pkt + length - chunk;
                        memmove(sess->hcs_rxbuffer, chunk, len);
                        /* for next read, read to offset 'len' of rxbuffer */
                        sess->hcs_rxbufoffset = len;
                    }
                    goto end;
                }

                *chunk_data = '\0'; /* NULL terminated so we can use string operation */
                chunk_size = get_chunksize((const char *)chunk);
                if (chunk_size <= 0) {
                    cb->state = (chunk_size == 0) ? HTTPC_RX_FINISHED : /* last chunk: we are done! */
                                    HTTPC_RX_ERROR_RX_PROCESS;          /* invalid chunk */
                    reset_sess = TRUE;
                    goto end;
                }

#ifdef HTTPC_DEBUG
                ++sess->total_chunks;
#endif

                chunk_data += 2; /* CRLF */
                if (chunk_data >= pkt + length) {
                    /* pkt                                  pkt + length
                     * |                                      |
                     * V                                      V
                     * +--------------------------------------+
                     * |                 1a;<ext_1>;<ext_2>\r\nabcdefghijklmnopqrstuvwxyz\r\n
                     * |                                       |<----- chunk_size ----->|
                     * +--------------------------------------+
                     *                   A                     A
                     *                   |                     |
                     *                  chunk                chunk_data
                     */
                    sess->remain_len = chunk_size;
                    sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
                    goto end;
                }

                if (chunk_data + chunk_size > pkt + length) {
                    /* case 1: only partial data section is in rxbuffer
                     *
                     * pkt                                  pkt + length
                     * |                                      |
                     * V                                      V
                     * +--------------------------------------+
                     * |<----------- length ----------------->|
                     * |      1a;<ext_1>;<ext_2>\r\nabcdefghijklmnopqrstuvwxyz\r\n
                     * |                            |<----- chunk_size ----->|
                     * |                            |<- len ->|<-remain_len->|
                     * +--------------------------------------+
                     *        A                     A
                     *        |                     |
                     *       chunk                chunk_data
                     *
                     */
                    sess->remain_len = chunk_data + chunk_size - (pkt + length);
                    len = pkt + length - chunk_data;
                } else {
                    /* All chunk data is in rxbuffer
                     *
                     *         chunk_data                    pkt + length
                     * case 2:    |<-- chunk_size -->|        |
                     *                               \r\n     |
                     *
                     *              chunk_data                |
                     * case 3:         |<-- chunk_size -->|   |
                     *                                    \r\n|
                     *
                     *                chunk_data              |
                     * case 4:           |<-- chunk_size -->| |
                     *                                      \r|\n
                     *
                     *                  chunk_data            |
                     * case 5:             |<-- chunk_size -->|
                     *                                        |\r\n
                     */
                    sess->remain_len = 0;
                    len = chunk_size;
                }

                /* Return the chunk to user via his registered callback */
                cb->state = HTTPC_RX_MORE_DATA;
                http_client_rx_cb(sess, cb->state, chunk_data, len);
                sess->hcs_contentlength += len;

                if (sess->remain_len != 0) {
                    sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
                    goto end;
                }

                chunk = chunk_data + chunk_size + 2; /* 2 for CRLF */
            }                                        /* while */

            sess->offset = chunk - (pkt + length);
            sess->hcs_rxbufoffset = 0; /* for next read, read to the beginning of rxbuffer */
        }
    } /* if (HTTPC_FLAG_IS_SET(sess, HCF_RESP_CONT)) */

end:
    sess->hcs_lasttime = HTTPCTICKS;

    if (reset_sess) {
        /* Clear the session */
        HTTPC_RESET_FLAG(sess, HCF_RECV_HEADER);
        HTTPC_RESET_FLAG(sess, HCF_CHUNKED);
        HTTPC_RESET_FLAG(sess, HCF_RESP_CONT);

        HTTPC_MOVETOSTATE(sess, HCS_CONNECTED);

        sess->offset = 0;
        sess->remain_len = 0;

        memset(sess->hcs_rxbuffer, 0, sess->hcs_rxbuf_len);
        sess->hcs_rxbufoffset = 0;

        if (sendFailEvent) {
            http_client_rx_cb(sess, cb->state, NULL, 0);
        }
        if (cb->state != HTTPC_RX_CHUNK_CONTINUE) {
            cb->state = 0;
            cb->pending_req = FALSE;
        }
    }
    return HTTPC_OK;
}

/* FUNCTION: http_client_readbuf()
 *
 * read packet and process socket error
 *
 * PARAMS: sess
 *
 * RETURNS: OK or ERROR code
 */
static int http_client_readbuf(httpclient_sess *sess)
{
    int readval;
    int err;
    int state;

    readval = http_client_readsock(sess);

    if (sess->isHttps)
        return readval;

    if (readval <= 0) /* error on socket? */
    {
        int len = (int)sizeof(int);

        getsockopt(sess->hcs_socket, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
    } else
        err = 0;

    if (readval <= 0) /* error on socket? */
    {
#ifdef CONFIG_QAT_OTA_DEMO
        if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
            /*no need to print when running OTA over HTTP*/
        } else
#endif
        {
            htdbgprintf("%s: readval:%d ERRNO:%d\n", __func__, readval, err);
        }

        if ((err == EWOULDBLOCK) || (err == ENOBUFS)) {
            readval = 0; /* No new data arrived */
        } else {
            /* Server has closed the connection */
            if (err == 0 ||                        /* for IPv6 */
                err == ECONNRESET || err == EPIPE) /* for IPv4 */
            {
                HTTPC_SET_FLAG(sess, HCF_CLOSED_BY_PEER);

                if (HTTPC_FLAG_IS_SET(sess, HCF_TUNNEL_ESTABLISHED)) {
                    state = HTTPC_RX_TUNNEL_CLOSED;
                    HTTPC_RESET_FLAG(sess, HCF_TUNNEL_ESTABLISHED);
                } else {
                    state = HTTPC_RX_ERROR_SERVER_CLOSED;
                }
            } else /* Treat other errno as internal read error */
            {
                state = HTTPC_RX_ERROR_CONNECTION_CLOSED;
            }

            /* Let the app know the state */
            http_client_rx_cb(sess, state, NULL, 0);

            http_client_disconnect(sess);
        }
    } else if (readval > 0) {
        sess->hcs_lasttime = HTTPCTICKS; /* reset activity timer */
        sess->hcs_rxbufoffset += readval;
        *(sess->hcs_rxbuffer + sess->hcs_rxbufoffset) = '\0';
        err = http_client_processpkt(sess, readval);
#ifdef CONFIG_QAT_OTA_DEMO
        if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
            /*no need to print when running OTA over HTTP*/
        } else
#endif
        {
            htdbgprintf("%s: readval:%d ERRNO:%d\n", __func__, readval, err);
        }
    }

    return (readval);
}

/* FUNCTION: http_check()
 *
 * SUPERLOOP implementation:
 * 10 - 100 times a second. select data and run httpc_timer.
 *
 * PARAMS: none
 *
 * RETURNS: none
 */
static void httpc_check(uint32_t param)
{
    fd_set httpc_fdrecv;
    httpclient_sess *session = NULL;
    int events = 0;
    int con_flag = 0;
    int maxfd = 0;
    uint32_t i;
    struct timeval tv;

    UNUSED(param);

    if (g_httpc_ctxt == NULL || g_httpc_ctxt->in_httpc_check) {
        qurt_thread_sleep(1000);
        return;
    }

    g_httpc_ctxt->in_httpc_check++;

    FD_ZERO(&httpc_fdrecv);

    qurt_mutex_lock(g_httpc_ctxt->lh);

    for (i = 0; i < httpc_max_num_con; i++) {
        if (!(httpclient_sess *)g_httpc_ctxt->httpc_sess[i])
            continue;

        session = (httpclient_sess *)g_httpc_ctxt->httpc_sess[i];
        if (!session)
            continue;

        if (session) {
            if (session->hcs_socket != INVALID_SOCKET && session->hcs_state == HCS_CONNECTED) {
                FD_SET(session->hcs_socket, &httpc_fdrecv);
                con_flag = 1;
                if (maxfd < session->hcs_socket)
                    maxfd = session->hcs_socket;
            }
        }
    }

    if (!con_flag) {
        qurt_mutex_unlock(g_httpc_ctxt->lh);
        qurt_thread_sleep(1000);
        g_httpc_ctxt->in_httpc_check--;
        return;
    }

    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    qurt_mutex_unlock(g_httpc_ctxt->lh);

    events = select(maxfd + 1, &httpc_fdrecv, NULL, NULL, &tv);

    if (events > 0) {
        for (i = 0; i < httpc_max_num_con; i++) {
            qurt_mutex_lock(g_httpc_ctxt->lh);
            session = (httpclient_sess *)g_httpc_ctxt->httpc_sess[i];
            if (session) {
                if (session->hcs_socket != INVALID_SOCKET && FD_ISSET(session->hcs_socket, &httpc_fdrecv)) {
                    if (HTTPC_IS_UPGRADED(session)) {
                        /* An HTTP connection can be upgraded to a different protocol,
                         * e.g. websocket, using the upgrade header.  If this flag is
                         * set, the RX callback is given raw bytes rather than the
                         * parsed HTTP response.
                         */
                        session->protocol_upgrade_module_rx_cb(session->protocol_upgrade_module_rx_cb_arg, session,
                                                               NULL, 0);
                    } else {
                        http_client_readbuf(session);
                    }
                }
            }
            qurt_mutex_unlock(g_httpc_ctxt->lh);
        }
    }

    if (TIME_EXP(TIME_ADD(g_httpc_ctxt->timer_lasttime, HTTPC_TIMER_TIMEOUT), HTTPCTICKS)) {
        http_client_timer();
        g_httpc_ctxt->timer_lasttime = HTTPCTICKS;
    }
    g_httpc_ctxt->in_httpc_check--;
    return;
}

/* The application thread works on a "controlled polling" basis:
 * it wakes up periodically and polls for work.
 *
 * The HTTP Server task could alternativly be set up to use blocking sockets,
 * in which case the loops below would only call the "xxx_check()"
 * routines - suspending would be handled by the TCP code.
 */

/* FUNCTION: tk_httpc()
 *
 * PARAM1: n/a
 *
 * RETURNS: n/a
 */
void http_client_task(void __attribute__((__unused__)) * pvParameters)
{
#if CONFIG_QAT_POWERSAVE_DEMO
    uint32 Signal_Waiting;
#endif
    for (;;) {
        httpc_check(0);
        if (httpc_thread_started == FALSE) {
            qurt_signal_set(&httpc_sem, 1);
        }
#if CONFIG_QAT_POWERSAVE_DEMO
        if (powersave_active) {
            qurt_signal_wait_timed(&http_sem, 1, QURT_SIGNAL_ATTR_CLEAR_MASK, &Signal_Waiting, QURT_TIME_WAIT_FOREVER);
        }
#endif
    }
}

#ifdef HTTPC_DEBUG
#pragma pop
#endif
