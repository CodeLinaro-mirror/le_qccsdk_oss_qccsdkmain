/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#define MBEDTLS_ALLOW_PRIVATE_ACCESS

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#define mbedtls_free   free
#define mbedtls_calloc calloc
#endif

#include "net_shell.h"
#include "ssl_demo.h"
#include "qapi_status.h"
#include "timer.h"
#include "safeAPI.h"

#include "mbedtls/ssl.h"
//#include "mbedtls/ssl_internal.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"

#ifdef CONFIG_NET_SSL_DEMO

#define SSL_CLIENT_PRINTF(...) printf(__VA_ARGS__)

#define ALPN_LIST_SIZE 10

#define MAX_STREAM 2

#define PUT_UINT64_BE(out_be, in_le, i)                              \
    {                                                                \
        (out_be)[(i) + 0] = (unsigned char)(((in_le) >> 56) & 0xFF); \
        (out_be)[(i) + 1] = (unsigned char)(((in_le) >> 48) & 0xFF); \
        (out_be)[(i) + 2] = (unsigned char)(((in_le) >> 40) & 0xFF); \
        (out_be)[(i) + 3] = (unsigned char)(((in_le) >> 32) & 0xFF); \
        (out_be)[(i) + 4] = (unsigned char)(((in_le) >> 24) & 0xFF); \
        (out_be)[(i) + 5] = (unsigned char)(((in_le) >> 16) & 0xFF); \
        (out_be)[(i) + 6] = (unsigned char)(((in_le) >> 8) & 0xFF);  \
        (out_be)[(i) + 7] = (unsigned char)(((in_le) >> 0) & 0xFF);  \
    }

/* Default option value */
#define DFL_SERVER_NAME      "localhost"
#define DFL_SERVER_ADDR      NULL
#define DFL_SERVER_PORT      "4433"
#define DFL_REQUEST_SIZE     34
#define DFL_DEBUG_LEVEL      0
#define DFL_CA_FILE          ""
#define DFL_CRT_FILE         ""
#define DFL_KEY_FILE         ""
#define DFL_FORCE_CIPHER     0
#define DFL_RENEGOTIATION    MBEDTLS_SSL_RENEGOTIATION_DISABLED
#define DFL_RENEGOTIATE      0
#define DFL_EXCHANGES        1
#define DFL_MIN_VERSION      -1
#define DFL_MAX_VERSION      -1
#define DFL_AUTH_MODE        -1
#define DFL_ALPN_STRING      NULL
#define DFL_DISPLAY_INTERVAL 0
#define DFL_TX_ONLY          0

#define DFL_RESPONSE_SIZE    200
#define DFL_CERT_REQ_CA_LIST MBEDTLS_SSL_CERT_REQ_CA_LIST_ENABLED
#define DFL_CRT_FILE2        ""
#define DFL_KEY_FILE2        ""
#define DFL_RENEGO_DELAY     -2
#define DFL_RENEGO_PERIOD    ((uint64_t)-1)
#define DFL_RX_ONLY          0

#define MAX_REQUEST_SIZE 8192
#define GET_REQUEST      "GET %s HTTP/1.0\r\nExtra-header: "
#define GET_REQUEST_END  "\r\n\r\n"
#define HTTP_RESPONSE                                    \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n"                  \
    "<p>Successful connection using: %s</p>\r\n"

uint8_t quit_ssl = 0;

uint8_t ssl_stream_id[MAX_STREAM] = {0};

const char *client_pers = "ssl_client2";
const char *server_pers = "ssl_server2";

const char demo_test_ca_crt_rsa[] =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIDCTCCAfECFEuk3qtD573tgBog04Qd6HODDQotMA0GCSqGSIb3DQEBCwUAMEEx\r\n"
    "CzAJBgNVBAYTAkNOMQswCQYDVQQIDAJTSDERMA8GA1UECgwIUXVhbGNvbW0xEjAQ\r\n"
    "BgNVBAMMCWxvY2FsaG9zdDAeFw0yMzA5MDEwOTQ5NDZaFw0zMzA4MjkwOTQ5NDZa\r\n"
    "MEExCzAJBgNVBAYTAkNOMQswCQYDVQQIDAJTSDERMA8GA1UECgwIUXVhbGNvbW0x\r\n"
    "EjAQBgNVBAMMCWxvY2FsaG9zdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC\r\n"
    "ggEBANwVE1P0eJKuhuQrTTVdl2WdhjE8Hs8Q/2w9gM7tVKcCYyRfO1HnAGIfu+dG\r\n"
    "gOUVfgHxle/0OG3W4DyKDes8e4xj9DP/1UachjvSVH0fKl3V9fp8sGrytsmGUOJx\r\n"
    "grozAn/0J+RkzTarontlwxv5LVCqw7m3IA3vE8dGnGmajVRK5NQ99ia+877q52u4\r\n"
    "epbQ461s7VwWS+bRDwiG9aFbiUNC+D9jz0FSlkCLYGAoT7MQQ7Jc75qPvsJAZ+rY\r\n"
    "ypiz28vp5CQwpPC+0hbW7M68O6aSYAwOrp4J5yA+Tuu8pOfZKgsSIx5adYLnxUM2\r\n"
    "YQ3M+qjD8l2+Gn22D8+kPDd5kz0CAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAfMGx\r\n"
    "xA0Pok4znJP+AFyTGCDKZGA/kT4wphCCuYalf810NCqUSBb5lX3C/DtAkznzZqhF\r\n"
    "m9k0Qm0VMGeEJzUjaHGOTUUXPpYyLEkfqQtcGc3TD2AX4eQVxTaekqCcLJWbt6u4\r\n"
    "eYjSXNw3AWzRU2b8NYC0KIr33EteQUE0QS0sp6IkEv31nMbNw7MabXYEca3zbLW8\r\n"
    "hhBWftw+MO1pkklQSXCzVtSw7Q1VFbaFD5YxDvqDUcc76yPaF57DIETUyMtUobna\r\n"
    "3a9Lsvw8Zt+kGMNux3VvE7902IYcMPyh8UzUu2/mnozJMaZXYVjU8putYWfFx2qq\r\n"
    "gCySfMNX4z9nvKsKuA==\r\n"
    "-----END CERTIFICATE-----\r\n";
const char demo_test_ca_key_rsa[] =
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIIEpQIBAAKCAQEA3BUTU/R4kq6G5CtNNV2XZZ2GMTwezxD/bD2Azu1UpwJjJF87\r\n"
    "UecAYh+750aA5RV+AfGV7/Q4bdbgPIoN6zx7jGP0M//VRpyGO9JUfR8qXdX1+nyw\r\n"
    "avK2yYZQ4nGCujMCf/Qn5GTNNquie2XDG/ktUKrDubcgDe8Tx0acaZqNVErk1D32\r\n"
    "Jr7zvurna7h6ltDjrWztXBZL5tEPCIb1oVuJQ0L4P2PPQVKWQItgYChPsxBDslzv\r\n"
    "mo++wkBn6tjKmLPby+nkJDCk8L7SFtbszrw7ppJgDA6ungnnID5O67yk59kqCxIj\r\n"
    "Hlp1gufFQzZhDcz6qMPyXb4afbYPz6Q8N3mTPQIDAQABAoIBAQDXLDfgbnXwG3jA\r\n"
    "3mE3WtDpxbBstLX+h0TjQ+KK7dlFC/14ky9BLVPfm90wCmt9Dp1LMzMADsuZAGvu\r\n"
    "ZJ+lLVYx9YvNx0RzLBfFpyd5yZ23brh29a/acIEr2Ql9y7Mfbz3zcfgKwk8tM3PB\r\n"
    "p8WxtNaMNtjz20oYtXWl8LB+Q2AIVvwzxbWn1YPa8+MzUh/iO6RCdIMu4YrzqRg4\r\n"
    "7VIGI1cQXrk5HcGE39dL9wcV5J4OJOqx98s5DROAbwu2wBpuejF8pAqrCGxXmZ4y\r\n"
    "4ydwUQ/QoefIo9H+24MnJqC+I+0NkHRPBm9hvnOXHLX/GaXPDJyvWyCaB0NBykRa\r\n"
    "Ga4i1IdhAoGBAPr8jez/UtErK2iKAoyGjJcqWG1cY/zqpsXdCGPsBalS5ZO3YXdY\r\n"
    "6rnky/jOc8XnsbnTv15KPSkSApZjzCp9eljkauTPKeN8jxz4CczzPX66jJqgVvkZ\r\n"
    "BY0LMQeLr5CLGr5EvIRSfMg1fE3B0A5UY8K4PVRMWjbkg1LOxOY9XPH1AoGBAOB6\r\n"
    "fTzRu/BUhOdXqcNskIUDcEYd+FrpBPlY7EnuPUNGkpiSKU2eDCK1busu6QJMU4cx\r\n"
    "dNrFtTT5ygJ0e9q2Gsqe83OhrRtzyXfWjnfzyNvMi2eoDnX6ton9ztgqnG1BktpB\r\n"
    "Xw022GC51N/2vFGYRKaClmIV6dbE7clZ+dHiwKcpAoGBAMQnK3iElyH4HiXGbnWL\r\n"
    "FkdyBcf6g/5/GTXcKBmHtWj+64OFtzvCFziPUsYx+5M5H9I+Zfp4BNKbS8BjYIX4\r\n"
    "qOzeH5iRO4iZqXOXenldxLrNauPR2gc2AfuYOopOJjjOLmlzaO31VaZW/r36cfMx\r\n"
    "CwJ8YRoHzh3Ge8f05zeVz5UdAoGBAKXXmOLwCKtbpfzMdS1d7b93dOE4jx2K/hPB\r\n"
    "sIBGNJiZcQCrKzyewVR7OoEiXR9HiIZe8XgXjPKggLAjosVIuK5tlGsfTSb0+ilB\r\n"
    "KGxSfVh126AvNs/O9EEqdECb6omFYptApJq8pEuBv1Xfke2uUzm5TKUWj3YOc6bI\r\n"
    "hJqdrbtRAoGAZjvAecQJMKbBM/G8bSuJ3rbZpKMfWJXfDdEjkIkL30DBPmia+Xl9\r\n"
    "bV0RpTmn5hhLxx7stPr+qAAzetf5ypoAB9yoDe93rLFf1CZgLCWVjcdr2rI7YjIN\r\n"
    "zru7SSRCWAJcddX7HPwv7NZqldVeV75ztkZhxfhdiQvreGeGG6ruKQw=\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

const char demo_test_ca_crt_ec[] =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIIBvDCCAWOgAwIBAgIUYAFkFzfdVCpLdEZUECqHEPLLj5AwCgYIKoZIzj0EAwIw\r\n"
    "NDELMAkGA1UEBhMCQ04xETAPBgNVBAoMCFF1YWxjb21tMRIwEAYDVQQDDAlsb2Nh\r\n"
    "bGhvc3QwHhcNMjMwODE0MDYxOTIxWhcNMzMwODExMDYxOTIxWjA0MQswCQYDVQQG\r\n"
    "EwJDTjERMA8GA1UECgwIUXVhbGNvbW0xEjAQBgNVBAMMCWxvY2FsaG9zdDBZMBMG\r\n"
    "ByqGSM49AgEGCCqGSM49AwEHA0IABJ0vVeChpSGX1NOEaDAZWtnEulNQGI69jRUY\r\n"
    "ZS0A2hN+Y3PW3G6ipV2nhvBHGs6PdG4vVADhFLx1UeHy+ujNvPKjUzBRMB0GA1Ud\r\n"
    "DgQWBBSAvQHIpvNgsOreMrDJ9wZObM6BbDAfBgNVHSMEGDAWgBSAvQHIpvNgsOre\r\n"
    "MrDJ9wZObM6BbDAPBgNVHRMBAf8EBTADAQH/MAoGCCqGSM49BAMCA0cAMEQCICEN\r\n"
    "OgHvl05+Be7AjIXjunNsn1OO12Df31kWl5+6RWcYAiBgyNitnuESvB2e/THmewPZ\r\n"
    "QXE4YMQ5b1FukX2r8Q7mdw==\r\n"
    "-----END CERTIFICATE-----\r\n";

const char demo_test_ca_key_ec[] =
    "-----BEGIN EC PRIVATE KEY-----\r\n"
    "MHcCAQEEIBY2Zk3U/p8q3bcB/BW0pXxU/fxLDI2vxPYcg5zjqqmSoAoGCCqGSM49\r\n"
    "AwEHoUQDQgAEnS9V4KGlIZfU04RoMBla2cS6U1AYjr2NFRhlLQDaE35jc9bcbqKl\r\n"
    "XaeG8Ecazo90bi9UAOEUvHVR4fL66M288g==\r\n"
    "-----END EC PRIVATE KEY-----\r\n";

const char *demo_test_cas[] = {
#if defined(MBEDTLS_RSA_C)
    demo_test_ca_crt_rsa,
#endif
#if defined(MBEDTLS_ECDSA_C)
    demo_test_ca_crt_ec,
#endif
    NULL};

const size_t demo_test_cas_len[] = {
#if defined(MBEDTLS_RSA_C)
    sizeof(demo_test_ca_crt_rsa),
#endif
#if defined(MBEDTLS_ECDSA_C)
    sizeof(demo_test_ca_crt_ec),
#endif
    0};

qapi_Status_t ssl_client_set_default_options(client_options *client_opt)
{
    client_opt->server_name = DFL_SERVER_NAME;
    client_opt->server_addr = DFL_SERVER_ADDR;
    client_opt->server_port = DFL_SERVER_PORT;
    client_opt->debug_level = DFL_DEBUG_LEVEL;
    client_opt->request_size = DFL_REQUEST_SIZE;
    client_opt->ca_file = DFL_CA_FILE;
    client_opt->crt_file = DFL_CRT_FILE;
    client_opt->key_file = DFL_KEY_FILE;
    client_opt->force_ciphersuite[0] = DFL_FORCE_CIPHER;
    client_opt->renegotiation = DFL_RENEGOTIATION;
    client_opt->renegotiate = DFL_RENEGOTIATE;
    client_opt->exchanges = DFL_EXCHANGES;
    client_opt->min_version = DFL_MIN_VERSION;
    client_opt->max_version = DFL_MAX_VERSION;
    client_opt->auth_mode = DFL_AUTH_MODE;
    client_opt->alpn_string = DFL_ALPN_STRING;
    client_opt->display_interval = DFL_DISPLAY_INTERVAL;
    client_opt->tx_only = DFL_TX_ONLY;

    return QAPI_OK;
}

qapi_Status_t ssl_server_set_default_options(server_options *server_opt)
{
    server_opt->server_addr = DFL_SERVER_ADDR;
    server_opt->server_port = DFL_SERVER_PORT;
    server_opt->debug_level = DFL_DEBUG_LEVEL;
    server_opt->response_size = DFL_RESPONSE_SIZE;
    server_opt->auth_mode = DFL_AUTH_MODE;
    server_opt->cert_req_ca_list = DFL_CERT_REQ_CA_LIST;
    server_opt->ca_file = DFL_CA_FILE;
    server_opt->crt_file = DFL_CRT_FILE;
    server_opt->key_file = DFL_KEY_FILE;
    server_opt->crt_file2 = DFL_CRT_FILE2;
    server_opt->key_file2 = DFL_KEY_FILE2;
    server_opt->force_ciphersuite[0] = DFL_FORCE_CIPHER;
    server_opt->renegotiation = DFL_RENEGOTIATION;
    server_opt->renegotiate = DFL_RENEGOTIATE;
    server_opt->renego_delay = DFL_RENEGO_DELAY;
    server_opt->renego_period = DFL_RENEGO_PERIOD;
    server_opt->alpn_string = DFL_ALPN_STRING;
    server_opt->min_version = DFL_MIN_VERSION;
    server_opt->max_version = DFL_MAX_VERSION;
    server_opt->exchanges = DFL_EXCHANGES;
    server_opt->display_interval = DFL_DISPLAY_INTERVAL;
    server_opt->rx_only = DFL_RX_ONLY;

    return QAPI_OK;
}

static uint8_t ssl_get_unused_id()
{
    uint8_t i;
    for (i = 0; i < MAX_STREAM; i++) {
        if (ssl_stream_id[i] == 0) {
            ssl_stream_id[i] = 1;
            break;
        }
    }
    return i;
}

static void demo_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    ((void)ctx);
    const char *p, *basename;

    /* Extract basename from file */
    for (p = basename = file; *p != '\0'; p++)
        if (*p == '/' || *p == '\\')
            basename = p + 1;

    SSL_CLIENT_PRINTF("%s:%04d: |%d| %s", basename, line, level, str);
}

void ssl_statistic_print_period(ssl_stats *stats)
{
    uint32_t sec_val1, sec_val2;
    sec_val1 = stats->display_sec;
    sec_val2 = stats->display_sec + stats->display_interval;

    SSL_CLIENT_PRINTF("  . [%3d] %2d.0-%2d.0 sec %d Bytes %d bits/sec \n", stats->ssl_stream_id, sec_val1, sec_val2,
                      (uint32_t)stats->bytes, (uint32_t)stats->throughput);

    stats->display_sec += stats->display_interval;
}

void ssl_statistic_print_final(ssl_stats *stats, uint32_t duration)
{
    uint32_t sec_val = duration / 1000 + 1;
    SSL_CLIENT_PRINTF("  . [%3d] 0.0-%2d.0 sec %d Bytes %d bits/sec \n", stats->ssl_stream_id, sec_val,
                      (uint32_t)stats->total_bytes, (uint32_t)stats->throughput);
}

qapi_Status_t ssl_client_help()
{
    const int *list;
    const mbedtls_ssl_ciphersuite_t *suite;

    SSL_CLIENT_PRINTF("\n usage: ssl_client param1 <value> param2 <value>...\n");
    SSL_CLIENT_PRINTF("\n acceptable parameters:\n");
    SSL_CLIENT_PRINTF("    server_name %%s      default: localhost\n");
    SSL_CLIENT_PRINTF("    server_addr %%s      default: given by name\n");
    SSL_CLIENT_PRINTF("    server_port %%d      default: 4433\n");
    SSL_CLIENT_PRINTF("    request_size %%d     default: about 34 (basic request)\n");
    SSL_CLIENT_PRINTF("                          (minimum: 0, max: %d)\n", MAX_REQUEST_SIZE);
    SSL_CLIENT_PRINTF("    debug_level %%d      default: 0 (disabled)\n");
    SSL_CLIENT_PRINTF("    auth_mode %%s        default: (library default: none)\n");
    SSL_CLIENT_PRINTF("    ca_file %%s          Not supported yet. Use preloaded CA.\n");
    SSL_CLIENT_PRINTF("    crt_file %%s         Not supported yet. Use preloaded Cert.\n");
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    SSL_CLIENT_PRINTF("    renegotiation %%d    default: 0 (disabled)\n");
    SSL_CLIENT_PRINTF("    renegotiate %%d      default: 0 (disabled)\n");
#endif
    SSL_CLIENT_PRINTF("    exchanges %%d        default: 1\n");
#if defined(MBEDTLS_SSL_ALPN)
    SSL_CLIENT_PRINTF("    alpn %%s             default: \"\" (disabled)\n");
    SSL_CLIENT_PRINTF("                        example: spdy/1,http/1.1\n");
#endif
    SSL_CLIENT_PRINTF("    force_version %%s    default: \"\" (none)\n");
    SSL_CLIENT_PRINTF("                        options: tls1_2\n");
    SSL_CLIENT_PRINTF("    display_interval %%d        default: 0\n");
    SSL_CLIENT_PRINTF("    tx_only %%d                 default: 0\n");
    SSL_CLIENT_PRINTF("    force_ciphersuite <name>   default: all enabled\n");
    SSL_CLIENT_PRINTF(" acceptable ciphersuite names:\n");

    list = mbedtls_ssl_list_ciphersuites();
    while (*list) {
        SSL_CLIENT_PRINTF(" %-42s", mbedtls_ssl_get_ciphersuite_name(*list));
        list++;

        if (!*list)
            break;
        suite = mbedtls_ssl_ciphersuite_from_id(*list);
        SSL_CLIENT_PRINTF(" %s\n", mbedtls_ssl_get_ciphersuite_name(*list));
        list++;
    }

    return QAPI_OK;
}

qapi_Status_t ssl_server_help()
{
    const int *list;
    const mbedtls_ssl_ciphersuite_t *suite;

    SSL_CLIENT_PRINTF("\n usage: ssl_server param1 <value> param2 <value>...\n");
    SSL_CLIENT_PRINTF("\n acceptable parameters:\n");
    SSL_CLIENT_PRINTF("    server_addr %%s      default: (all interfaces)\n");
    SSL_CLIENT_PRINTF("    server_port %%d      default: 4433\n");
    SSL_CLIENT_PRINTF("    debug_level %%d      default: 0 (disabled)\n");
    SSL_CLIENT_PRINTF("    response_size %%d    default: about 152 (basic response)\n");
    SSL_CLIENT_PRINTF("                          (minimum: 0, max: %d)\n", MAX_REQUEST_SIZE);
    SSL_CLIENT_PRINTF("    auth_mode %%s        default: (library default: none)\n");
    SSL_CLIENT_PRINTF("    cert_req_ca_list %%d default: 1 (send ca list)\n");
    SSL_CLIENT_PRINTF("                        options: 1 (send ca list), 0 (don't send)\n");
    SSL_CLIENT_PRINTF("    ca_file %%s          Not supported yet. Use preloaded CA.\n");
    SSL_CLIENT_PRINTF("    crt_file %%s         Not supported yet. Use preloaded Cert.\n");
    SSL_CLIENT_PRINTF("    key_file %%s         Not supported yet. Use preloaded key.\n");
    SSL_CLIENT_PRINTF("    crt_file2 %%s        Not supported yet. Use preloaded Cert.\n");
    SSL_CLIENT_PRINTF("    key_file2 %%s        Not supported yet. Use preloaded key.\n");
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    SSL_CLIENT_PRINTF("    renegotiation %%d    default: 0 (disabled)\n");
    SSL_CLIENT_PRINTF("    renegotiate %%d      default: 0 (disabled)\n");
    SSL_CLIENT_PRINTF("    renego_delay %%d     default: -2 (library default)\n");
    SSL_CLIENT_PRINTF("    renego_period %%d    default: (2^64 - 1)\n");
#endif
    SSL_CLIENT_PRINTF("    exchanges %%d        default: 1\n");
#if defined(MBEDTLS_SSL_ALPN)
    SSL_CLIENT_PRINTF("    alpn %%s             default: \"\" (disabled)\n");
    SSL_CLIENT_PRINTF("                        example: spdy/1,http/1.1\n");
#endif
    SSL_CLIENT_PRINTF("    force_version %%s    default: \"\" (none)\n");
    SSL_CLIENT_PRINTF("                        options: tls1_2\n");
    SSL_CLIENT_PRINTF("    display_interval %%d        default: 0\n");
    SSL_CLIENT_PRINTF("    rx_only %%d                 default: 0\n");
    SSL_CLIENT_PRINTF("    force_ciphersuite <name>   default: all enabled\n");
    SSL_CLIENT_PRINTF(" acceptable ciphersuite names:\n");

    list = mbedtls_ssl_list_ciphersuites();
    while (*list) {
        SSL_CLIENT_PRINTF(" %-42s", mbedtls_ssl_get_ciphersuite_name(*list));
        list++;

        if (!*list)
            break;
        suite = mbedtls_ssl_ciphersuite_from_id(*list);
        SSL_CLIENT_PRINTF(" %s\n", mbedtls_ssl_get_ciphersuite_name(*list));
        list++;
    }

    return QAPI_OK;
}

qapi_Status_t ssl_client_parse_paramters(client_options *client_opt, uint32_t Parameter_Count,
                                         QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t argn;

    ssl_client_set_default_options(client_opt);

    for (argn = 0; argn < Parameter_Count; argn++) {
        if (argn == Parameter_Count - 1) {
            SSL_CLIENT_PRINTF("What is value of %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        }

        else if (0 == strcmp("server_name", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->server_name = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("server_addr", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->server_addr = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("server_port", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->server_port = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("debug_level", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->debug_level = Parameter_List[argn].Integer_Value;
            if (client_opt->debug_level < 0 || client_opt->debug_level > 65535) {
                return QAPI_ERROR;
            }
        }

        else if (0 == strcmp("request_size", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->request_size = Parameter_List[argn].Integer_Value;
            if (client_opt->request_size < 0 || client_opt->request_size > MAX_REQUEST_SIZE)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("ca_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: CA config not supported yet. Will use the preloaded CA.\n");
        }

        else if (0 == strcmp("crt_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Cert config not supported yet. Will use the preloaded cert.\n");
        }

        else if (0 == strcmp("key_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Key config not supported yet. Will use the preloaded key.\n");
        }

        else if (0 == strcmp("force_ciphersuite", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id(Parameter_List[argn].String_Value);

            if (client_opt->force_ciphersuite[0] == 0)
                return QAPI_ERROR;

            client_opt->force_ciphersuite[1] = 0;
        }

        else if (0 == strcmp("renegotiation", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->renegotiation = (Parameter_List[argn].Integer_Value) ? MBEDTLS_SSL_RENEGOTIATION_ENABLED
                                                                             : MBEDTLS_SSL_RENEGOTIATION_DISABLED;
        }

        else if (0 == strcmp("renegotiate", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->renegotiate = Parameter_List[argn].Integer_Value;
            if (client_opt->renegotiate < 0 || client_opt->renegotiate > 1)
                return QAPI_ERROR;
        } else if (0 == strcmp("exchanges", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->exchanges = Parameter_List[argn].Integer_Value;
            if (client_opt->exchanges < 0)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("alpn", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->alpn_string = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("force_version", Parameter_List[argn].String_Value)) {
            argn++;

            if (strcmp(Parameter_List[argn].String_Value, "tls1_2") == 0) {
                client_opt->min_version = MBEDTLS_SSL_MINOR_VERSION_3;
                client_opt->max_version = MBEDTLS_SSL_MINOR_VERSION_3;
            } else
                return QAPI_ERROR;
        } else if (0 == strcmp("auth_mode", Parameter_List[argn].String_Value)) {
            argn++;

            if (strcmp(Parameter_List[argn].String_Value, "none") == 0)
                client_opt->auth_mode = MBEDTLS_SSL_VERIFY_NONE;
            else if (strcmp(Parameter_List[argn].String_Value, "optional") == 0)
                client_opt->auth_mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
            else if (strcmp(Parameter_List[argn].String_Value, "required") == 0)
                client_opt->auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;
            else
                return QAPI_ERROR;
        } else if (0 == strcmp("display_interval", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->display_interval = Parameter_List[argn].Integer_Value;
            if (client_opt->display_interval < 0)
                return QAPI_ERROR;
        } else if (0 == strcmp("tx_only", Parameter_List[argn].String_Value)) {
            argn++;

            client_opt->tx_only = Parameter_List[argn].Integer_Value;
            if (client_opt->tx_only < 0 || client_opt->tx_only > 1)
                return QAPI_ERROR;
        } else {
            SSL_CLIENT_PRINTF("What is %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        }
    }

    return QAPI_OK;
}

qapi_Status_t ssl_server_parse_paramters(server_options *server_opt, uint32_t Parameter_Count,
                                         QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t argn;

    ssl_server_set_default_options(server_opt);

    for (argn = 0; argn < Parameter_Count; argn++) {
        if (argn == Parameter_Count - 1) {
            SSL_CLIENT_PRINTF("What is value of %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        }

        else if (0 == strcmp("server_addr", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->server_addr = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("server_port", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->server_port = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("debug_level", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->debug_level = Parameter_List[argn].Integer_Value;
            if (server_opt->debug_level < 0 || server_opt->debug_level > 65535) {
                return QAPI_ERROR;
            }
        }

        else if (0 == strcmp("response_size", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->response_size = Parameter_List[argn].Integer_Value;
            if (server_opt->response_size < 0 || server_opt->response_size > MAX_REQUEST_SIZE)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("auth_mode", Parameter_List[argn].String_Value)) {
            argn++;

            if (strcmp(Parameter_List[argn].String_Value, "none") == 0)
                server_opt->auth_mode = MBEDTLS_SSL_VERIFY_NONE;
            else if (strcmp(Parameter_List[argn].String_Value, "optional") == 0)
                server_opt->auth_mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
            else if (strcmp(Parameter_List[argn].String_Value, "required") == 0)
                server_opt->auth_mode = MBEDTLS_SSL_VERIFY_REQUIRED;
            else
                return QAPI_ERROR;
        }

        else if (0 == strcmp("cert_req_ca_list", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->cert_req_ca_list = Parameter_List[argn].Integer_Value;
            if (server_opt->cert_req_ca_list < 0 || server_opt->cert_req_ca_list > 1)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("ca_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: CA config not supported yet. Will use the preloaded CA.\n");
        }

        else if (0 == strcmp("crt_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Cert config not supported yet. Will use the preloaded cert.\n");
        }

        else if (0 == strcmp("key_file", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Key config not supported yet. Will use the preloaded key.\n");
        }

        else if (0 == strcmp("crt_file2", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Cert config not supported yet. Will use the preloaded cert.\n");
        }

        else if (0 == strcmp("key_file2", Parameter_List[argn].String_Value)) {
            argn++;

            SSL_CLIENT_PRINTF("ERROR: Key config not supported yet. Will use the preloaded key.\n");
        }

        else if (0 == strcmp("renegotiation", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->renegotiation = (Parameter_List[argn].Integer_Value) ? MBEDTLS_SSL_RENEGOTIATION_ENABLED
                                                                             : MBEDTLS_SSL_RENEGOTIATION_DISABLED;
        }

        else if (0 == strcmp("renegotiate", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->renegotiate = Parameter_List[argn].Integer_Value;
            if (server_opt->renegotiate < 0 || server_opt->renegotiate > 1)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("renego_delay", Parameter_List[argn].String_Value)) {
            argn++;
            server_opt->renego_delay = Parameter_List[argn].Integer_Value;
        }

        else if (0 == strcmp("renego_period", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->renego_period = 0;
            for (size_t index = 0; index < strlen(Parameter_List[argn].String_Value); index++) {
                if ((Parameter_List[argn].String_Value[index] < '0') ||
                    (Parameter_List[argn].String_Value[index] > '9'))
                    return QAPI_ERROR;
                server_opt->renego_period =
                    server_opt->renego_period * 10 + (Parameter_List[argn].String_Value[index] - '0');
            }

            if (server_opt->renego_period < 2)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("alpn", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->alpn_string = Parameter_List[argn].String_Value;
        }

        else if (0 == strcmp("force_version", Parameter_List[argn].String_Value)) {
            argn++;

            if (strcmp(Parameter_List[argn].String_Value, "tls1_2") == 0) {
                server_opt->min_version = MBEDTLS_SSL_MINOR_VERSION_3;
                server_opt->max_version = MBEDTLS_SSL_MINOR_VERSION_3;
            } else
                return QAPI_ERROR;
        }

        else if (0 == strcmp("exchanges", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->exchanges = Parameter_List[argn].Integer_Value;
            if (server_opt->exchanges < 0)
                return QAPI_ERROR;
        }

        else if (0 == strcmp("force_ciphersuite", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id(Parameter_List[argn].String_Value);

            if (server_opt->force_ciphersuite[0] == 0)
                return QAPI_ERROR;

            server_opt->force_ciphersuite[1] = 0;
        }

        else if (0 == strcmp("display_interval", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->display_interval = Parameter_List[argn].Integer_Value;
            if (server_opt->display_interval < 0)
                return QAPI_ERROR;
        } else if (0 == strcmp("rx_only", Parameter_List[argn].String_Value)) {
            argn++;

            server_opt->rx_only = Parameter_List[argn].Integer_Value;
            if (server_opt->rx_only < 0 || server_opt->rx_only > 1)
                return QAPI_ERROR;
        } else {
            SSL_CLIENT_PRINTF("What is %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        }
    }

    return QAPI_OK;
}

qapi_Status_t ssl_quit(uint32_t __attribute__((__unused__)) Parameter_Count,
                       QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    quit_ssl = 1;
    return QAPI_OK;
}

qapi_Status_t ssl_client(uint32_t __attribute__((__unused__)) Parameter_Count,
                         QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    uint8_t *buf = 0;
#if defined(MBEDTLS_SSL_ALPN)
    char *p;
#endif
    int ret = 0, i, len, tail_len, written, frags, buffer_size, new_session = 0;
    uint32_t flags, now, begin_time = 0, end_time = 0, ssl_display_next = 0, ssl_display_last = 0;

    mbedtls_net_context server_fd;
#if defined(MBEDTLS_SSL_ALPN)
    const char *alpn_list[ALPN_LIST_SIZE];
#endif
    mbedtls_entropy_context *entropy = 0;
    mbedtls_ctr_drbg_context *ctr_drbg = 0;
    mbedtls_ssl_context *ssl = 0;
    mbedtls_ssl_config *conf = 0;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt *cacert = 0, *clicert = 0;
    mbedtls_pk_context pkey;
#endif
    client_options *client_opt = 0;
    ssl_stats *stats = 0;

    mbedtls_net_init(&server_fd);
    /* Memory alloc */
    entropy = (mbedtls_entropy_context *)mbedtls_calloc(1, sizeof(mbedtls_entropy_context));
    if (!entropy) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_entropy_context));
        goto exit;
    }
    ctr_drbg = (mbedtls_ctr_drbg_context *)mbedtls_calloc(1, sizeof(mbedtls_ctr_drbg_context));
    if (!ctr_drbg) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ctr_drbg_context));
        goto exit;
    }
    ssl = (mbedtls_ssl_context *)mbedtls_calloc(1, sizeof(mbedtls_ssl_context));
    if (!ssl) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ssl_context));
        goto exit;
    }
    conf = (mbedtls_ssl_config *)mbedtls_calloc(1, sizeof(mbedtls_ssl_config));
    if (!conf) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ssl_config));
        goto exit;
    }
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    cacert = (mbedtls_x509_crt *)mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (!cacert) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_x509_crt));
        goto exit;
    }
    clicert = (mbedtls_x509_crt *)mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (!clicert) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_x509_crt));
        goto exit;
    }
#endif
    client_opt = (client_options *)mbedtls_calloc(1, sizeof(client_options));
    if (!client_opt) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(client_options));
        goto exit;
    }
    memset(client_opt, 0, sizeof(client_options));

    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_init(cacert);
    mbedtls_x509_crt_init(clicert);
    mbedtls_pk_init(&pkey);
#endif
#if defined(MBEDTLS_SSL_ALPN)
    memset((void *)alpn_list, 0, sizeof(alpn_list));
#endif

    if (0 == strncmp("help", Parameter_List[0].String_Value, strlen(Parameter_List[0].String_Value)))
        goto usage;

    if (QAPI_OK != ssl_client_parse_paramters(client_opt, Parameter_Count, Parameter_List))
        goto usage;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(client_opt->debug_level);
#endif

    if (client_opt->request_size < DFL_REQUEST_SIZE)
        buffer_size = DFL_REQUEST_SIZE + 1;
    else
        buffer_size = client_opt->request_size + 1;

    buf = mbedtls_calloc(1, buffer_size);
    if (buf == NULL) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", buffer_size);
        goto exit;
    }

    stats = mbedtls_calloc(1, sizeof(ssl_stats));
    if (stats == NULL) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(ssl_stats));
        goto exit;
    }
    memset(stats, 0, sizeof(ssl_stats));

    if (client_opt->force_ciphersuite[0] > 0) {
        const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
        ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(client_opt->force_ciphersuite[0]);

        if (ciphersuite_info == NULL) {
            SSL_CLIENT_PRINTF("cannot find ciphersuite_info for %d id\n", client_opt->force_ciphersuite[0]);
            goto usage;
        }

        if (client_opt->max_version != -1 && ciphersuite_info->min_tls_version > client_opt->max_version) {
            SSL_CLIENT_PRINTF("forced ciphersuite not allowed with this protocol version\n");
            goto usage;
        }
        if (client_opt->min_version != -1 && ciphersuite_info->max_tls_version < client_opt->min_version) {
            SSL_CLIENT_PRINTF("forced ciphersuite not allowed with this protocol version\n");
            goto usage;
        }

        /* If the server selects a version that's not supported by
         * this suite, then there will be no common ciphersuite... */
        if (client_opt->max_version == -1 || client_opt->max_version > ciphersuite_info->max_tls_version) {
            client_opt->max_version = ciphersuite_info->max_tls_version;
        }
        if (client_opt->min_version < ciphersuite_info->min_tls_version) {
            client_opt->min_version = ciphersuite_info->min_tls_version;
        }
    }

#if defined(MBEDTLS_SSL_ALPN)
    if (client_opt->alpn_string != NULL) {
        p = (char *)client_opt->alpn_string;
        i = 0;

        /* Leave room for a final NULL in alpn_list */
        while (i < ALPN_LIST_SIZE - 1 && *p != '\0') {
            alpn_list[i++] = p;

            /* Terminate the current string and move on to next one */
            while (*p != ',' && *p != '\0')
                p++;
            if (*p == ',')
                *p++ = '\0';
        }
    }
#endif /* MBEDTLS_SSL_ALPN */

    stats->ssl_stream_id = ssl_get_unused_id();
    if (stats->ssl_stream_id == MAX_STREAM) {
        SSL_CLIENT_PRINTF("Reach the limited streams\n");
        goto exit;
    }
    new_session = 1;

    /*
     * 0. Initialize the RNG and the session data
     */
    quit_ssl = 0;

    SSL_CLIENT_PRINTF("\n  . Seeding the random number generator...");
    mbedtls_entropy_init(entropy);
    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy, (const unsigned char *)client_pers,
                                     strlen(client_pers))) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 1.1. Load the trusted CA
     */
    SSL_CLIENT_PRINTF("  . Loading the CA root certificate ...");

    for (i = 0; demo_test_cas[i] != NULL; i++) {
        ret = mbedtls_x509_crt_parse(cacert, (const unsigned char *)demo_test_cas[i], demo_test_cas_len[i]);
        if (ret != 0)
            break;
    }

    if (ret < 0) {
        SSL_CLIENT_PRINTF(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok \n");

    /*
     * 1.2. Load own certificate and private key
     *
     * (can be skipped if client authentication is not required)
     */
    SSL_CLIENT_PRINTF("  . Loading the client cert. and key...");

    ret = mbedtls_x509_crt_parse(clicert, (const unsigned char *)demo_test_ca_crt_ec, sizeof(demo_test_ca_crt_ec));

    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        goto exit;
    }

    ret = mbedtls_pk_parse_key(&pkey, (const unsigned char *)demo_test_ca_key_ec, sizeof(demo_test_ca_key_ec), NULL, 0,
                               mbedtls_ctr_drbg_random, ctr_drbg);

    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_pk_parse_key returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    /*
     * 2. Start the connection
     */
    if (client_opt->server_addr == NULL)
        client_opt->server_addr = client_opt->server_name;

    SSL_CLIENT_PRINTF("  . Connecting to tcp/%s/%s...", client_opt->server_addr, client_opt->server_port);

    if ((ret = mbedtls_net_connect(&server_fd, client_opt->server_addr, client_opt->server_port,
                                   MBEDTLS_NET_PROTO_TCP)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_net_connect returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");

    /*
     * 3. Setup stuff
     */
    SSL_CLIENT_PRINTF("  . Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret);
        goto exit;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    // if( client_opt.debug_level > 0 )
    // mbedtls_ssl_conf_verify( conf, my_verify, NULL );
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    if (client_opt->auth_mode != DFL_AUTH_MODE)
        mbedtls_ssl_conf_authmode(conf, client_opt->auth_mode);

#if defined(MBEDTLS_SSL_ALPN)
    if (client_opt->alpn_string != NULL)
        if ((ret = mbedtls_ssl_conf_alpn_protocols(conf, alpn_list)) != 0) {
            SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_conf_alpn_protocols returned %d\n\n", ret);
            goto exit;
        }
#endif

    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
    mbedtls_ssl_conf_dbg(conf, demo_debug, NULL);

    mbedtls_ssl_conf_read_timeout(conf, 5000);

    if (client_opt->force_ciphersuite[0] != DFL_FORCE_CIPHER)
        mbedtls_ssl_conf_ciphersuites(conf, client_opt->force_ciphersuite);

#if defined(MBEDTLS_SSL_RENEGOTIATION)
    mbedtls_ssl_conf_renegotiation(conf, client_opt->renegotiation);
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);

    if ((ret = mbedtls_ssl_conf_own_cert(conf, clicert, &pkey)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }
#endif

    if (client_opt->min_version != DFL_MIN_VERSION)
        mbedtls_ssl_conf_min_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, client_opt->min_version);

    if (client_opt->max_version != DFL_MAX_VERSION)
        mbedtls_ssl_conf_max_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, client_opt->max_version);

    if ((ret = mbedtls_ssl_setup(ssl, conf)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if ((ret = mbedtls_ssl_set_hostname(ssl, client_opt->server_name)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }
#endif

    mbedtls_ssl_set_bio(ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    SSL_CLIENT_PRINTF(" ok\n");

    /*
     * 4. Handshake
     */
    SSL_CLIENT_PRINTF("  . Performing the SSL/TLS handshake...");
    begin_time = hres_timer_curr_time_ms();

    while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n", -ret);
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
                SSL_CLIENT_PRINTF(
                    "    Unable to verify the server's certificate. "
                    "Either it is invalid,\n"
                    "    or you didn't set ca_file or ca_path "
                    "to an appropriate value.\n"
                    "    Alternatively, you may want to use "
                    "auth_mode=optional for testing purposes.\n");
            SSL_CLIENT_PRINTF("\n");
            goto exit;
        }
    }

    end_time = hres_timer_curr_time_ms();
    SSL_CLIENT_PRINTF("ok\n  . SSL handshake consumes %d.%d seconds\n", (end_time - begin_time) / 1000,
                      (end_time - begin_time) % 1000);
    const char *ciphersuite_info = mbedtls_ssl_get_ciphersuite(ssl);
    if (ciphersuite_info) {
        SSL_CLIENT_PRINTF("    [ Ciphersuite is %s ]\n", ciphersuite_info);
    } else {
        SSL_CLIENT_PRINTF("    [ Ciphersuite is NULL ]\n");
    }

    if ((ret = mbedtls_ssl_get_record_expansion(ssl)) >= 0)
        SSL_CLIENT_PRINTF("	 [ Record expansion is %d ]\n", ret);
    else
        SSL_CLIENT_PRINTF("	 [ Record expansion is unknown (compression) ]\n");

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    SSL_CLIENT_PRINTF("	 [ Maximum fragment length is %u ]\n", (unsigned int)mbedtls_ssl_get_max_frag_len(ssl));
#endif

#if defined(MBEDTLS_SSL_ALPN)
    if (client_opt->alpn_string != NULL) {
        const char *alp = mbedtls_ssl_get_alpn_protocol(ssl);
        SSL_CLIENT_PRINTF("	 [ Application Layer Protocol is %s ]\n", alp ? alp : "(none)");
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 5. Verify the server certificate
     */
    SSL_CLIENT_PRINTF("  . Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(ssl)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n");
    } else
        SSL_CLIENT_PRINTF(" ok\n");

#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_SSL_RENEGOTIATION)
    if (client_opt->renegotiate) {
        /*
         * Perform renegotiation (this must be done when the server is waiting
         * for input from our side).
         */
        SSL_CLIENT_PRINTF("  . Performing renegotiation...");
        while ((ret = mbedtls_ssl_renegotiate(ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_renegotiate returned %d\n\n", ret);
                goto exit;
            }
        }
        SSL_CLIENT_PRINTF(" ok\n");
    }
#endif /* MBEDTLS_SSL_RENEGOTIATION */

    stats->display_interval = (uint32_t)client_opt->display_interval;
    stats->exchanges = client_opt->exchanges;
    begin_time = hres_timer_curr_time_ms();
    ssl_display_last = hres_timer_curr_time_ms();
    ssl_display_next = ssl_display_last + 1000 * stats->display_interval;
    /*
     * 6. Write the GET request
     */
send_request:
    if (quit_ssl) {
        ret = 0;
        SSL_CLIENT_PRINTF(" Receive quit command\n");
        goto statistics;
    }

    if (client_opt->debug_level > 0)
        SSL_CLIENT_PRINTF("  > Write to server:");

    len = snprintf((char *)buf, buffer_size - 1, GET_REQUEST, "/");
    tail_len = (int)strlen(GET_REQUEST_END);

    /* Add padding to GET request to reach client_opt.request_size in length */
    if (client_opt->request_size != DFL_REQUEST_SIZE && len + tail_len < client_opt->request_size) {
        memset(buf + len, 'A', client_opt->request_size - len - tail_len);
        len += client_opt->request_size - len - tail_len;
    }

    strlcpy((char *)buf + len, GET_REQUEST_END, buffer_size - len);
    len += tail_len;

    /* Truncate if request size is smaller than the "natural" size */
    if (client_opt->request_size != DFL_REQUEST_SIZE && len > client_opt->request_size) {
        len = client_opt->request_size;

        /* Still end with \r\n unless that's really not possible */
        if (len >= 2)
            buf[len - 2] = '\r';
        if (len >= 1)
            buf[len - 1] = '\n';
    }

    for (written = 0, frags = 0; written < len; written += ret, frags++) {
        while ((ret = mbedtls_ssl_write(ssl, buf + written, len - written)) <= 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_write returned -0x%x\n\n", -ret);
                goto statistics;
            }
        }
    }
    buf[written] = '\0';
    if (client_opt->debug_level > 0)
        SSL_CLIENT_PRINTF(" %d bytes written in %d fragments\n\n%s\n", written, frags, (char *)buf);
    stats->bytes += written;
    stats->total_bytes += written;

    now = hres_timer_curr_time_ms();
    if ((stats->display_interval > 0) && (now >= ssl_display_next)) {
        stats->throughput = stats->bytes * 8 * 1000 / (now - ssl_display_last);
        ssl_statistic_print_period(stats);
        ssl_display_last = now;
        ssl_display_next = now + 1000 * stats->display_interval;
        stats->bytes = 0;
    }

    /*
     * 7. Read the HTTP response
     */
    if (client_opt->debug_level > 0 && !client_opt->tx_only)
        SSL_CLIENT_PRINTF("  < Read from server:");

    do {
        if (client_opt->tx_only)
            break;

        if (quit_ssl) {
            ret = 0;
            SSL_CLIENT_PRINTF(" Receive quit command\n");
            goto close_notify;
        }

        len = buffer_size - 1;
        memset(buf, 0, buffer_size);
        ret = mbedtls_ssl_read(ssl, buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_TIMEOUT)
            continue;

        if (ret <= 0) {
            switch (ret) {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    SSL_CLIENT_PRINTF(" connection was closed gracefully\n");
                    ret = 0;
                    goto close_notify;

                case 0:
                case MBEDTLS_ERR_NET_CONN_RESET:
                    SSL_CLIENT_PRINTF(" connection was reset by peer\n");
                    ret = 0;
                    goto statistics;

                default:
                    SSL_CLIENT_PRINTF(" mbedtls_ssl_read returned -0x%x\n", -ret);
                    goto statistics;
            }
        }

        if (mbedtls_ssl_get_bytes_avail(ssl) == 0) {
            len = ret;
            buf[len] = '\0';
            if (client_opt->debug_level > 0)
                SSL_CLIENT_PRINTF(" %d bytes read\n\n%s\n", len, (char *)buf);

            /* End of message should be detected according to the syntax of the
             * application protocol (eg HTTP), just use a dummy test here. */
            if (ret > 0 && buf[len - 1] == '\n') {
                ret = 0;
                break;
            }
        } else {
            int extra_len, ori_len;
            uint8_t *larger_buf = 0, *tmp_buf;

            ori_len = ret;
            extra_len = (int)mbedtls_ssl_get_bytes_avail(ssl);

            larger_buf = mbedtls_calloc(1, ori_len + extra_len + 1);
            if (larger_buf == NULL) {
                SSL_CLIENT_PRINTF("  ! memory allocation failed\n");
                ret = 1;
                goto statistics;
            }

            memset(larger_buf, 0, ori_len + extra_len + 1);
            memscpy(larger_buf, ori_len + extra_len + 1, buf, ori_len);

            tmp_buf = buf;
            buf = larger_buf;
            buffer_size = ori_len + extra_len + 1;
            mbedtls_free(tmp_buf);

            /* This read should never fail and get the whole cached data */
            ret = mbedtls_ssl_read(ssl, buf + ori_len, extra_len);
            if (ret != extra_len || mbedtls_ssl_get_bytes_avail(ssl) != 0) {
                SSL_CLIENT_PRINTF("  ! mbedtls_ssl_read failed on cached data\n");
                ret = 1;
                goto statistics;
            }

            buf[ori_len + extra_len] = '\0';
            if (client_opt->debug_level > 0)
                SSL_CLIENT_PRINTF(" %u bytes read (%u + %u)\n\n%s\n", ori_len + extra_len, ori_len, extra_len,
                                  (char *)buf);

            /* End of message should be detected according to the syntax of the
             * application protocol (eg HTTP), just use a dummy test here. */
            if (buf[ori_len + extra_len - 1] == '\n') {
                ret = 0;
                break;
            }
        }
    } while (1);

    /*
     * 7a. Continue doing data exchanges?
     */
    if (--stats->exchanges != 0)
        goto send_request;

    /*
     * 8. Done, cleanly close the connection
     */
close_notify:
    SSL_CLIENT_PRINTF("  . Closing the connection...");

    /* No error checking, the connection might be closed already */
    do
        ret = mbedtls_ssl_close_notify(ssl);
    while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
    ret = 0;

    SSL_CLIENT_PRINTF(" done\n");

statistics:
    if ((uint32_t)client_opt->exchanges != stats->exchanges) {
        end_time = hres_timer_curr_time_ms();
        stats->throughput = stats->total_bytes * 8 * 1000 / (end_time - begin_time);
        ssl_statistic_print_final(stats, end_time - begin_time);
    }

exit:
    if (buf) {
        mbedtls_free(buf);
    }

    mbedtls_net_free(&server_fd);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (clicert) {
        mbedtls_x509_crt_free(clicert);
        free(clicert);
    }
    if (cacert) {
        mbedtls_x509_crt_free(cacert);
        free(cacert);
    }
    mbedtls_pk_free(&pkey);
#endif
    if (ssl) {
        mbedtls_ssl_free(ssl);
        free(ssl);
    }
    if (conf) {
        mbedtls_ssl_config_free(conf);
        free(conf);
    }
    if (ctr_drbg) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        free(ctr_drbg);
    }
    if (entropy) {
        mbedtls_entropy_free(entropy);
        free(entropy);
    }

    if (new_session)
        ssl_stream_id[stats->ssl_stream_id] = 0;

    if (stats) {
        mbedtls_free(stats);
    }

    if (client_opt) {
        mbedtls_free(client_opt);
    }

    if (!ret)
        return QAPI_OK;
    else {
        SSL_CLIENT_PRINTF(" Function fail.\n");
        return QAPI_ERROR;
    }

usage:
    ssl_client_help();
    goto exit;
}

qapi_Status_t ssl_server(uint32_t __attribute__((__unused__)) Parameter_Count,
                         QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    uint8_t *buf = 0;
#if defined(MBEDTLS_SSL_ALPN)
    char *p;
#endif
    int ret = 0, i, len, written, frags, buffer_size, request_size, new_session = 0;
    uint32_t flags, now, begin_time = 0, end_time = 0, ssl_display_next = 0, ssl_display_last = 0;
    struct timeval tv = {0};

    mbedtls_net_context client_fd, listen_fd;
    fd_set rd_set;
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    unsigned char renego_period[8] = {0};
#endif
#if defined(MBEDTLS_SSL_ALPN)
    const char *alpn_list[ALPN_LIST_SIZE];
#endif
    mbedtls_entropy_context *entropy = 0;
    mbedtls_ctr_drbg_context *ctr_drbg = 0;
    mbedtls_ssl_context *ssl = 0;
    mbedtls_ssl_config *conf = 0;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt *cacert = 0, *srvcert = 0, *srvcert2 = 0;
    mbedtls_pk_context pkey, pkey2;
#endif
    server_options *server_opt = 0;
    ssl_stats *stats = 0;

    mbedtls_net_init(&client_fd);
    mbedtls_net_init(&listen_fd);
    /* Memory alloc */
    entropy = (mbedtls_entropy_context *)mbedtls_calloc(1, sizeof(mbedtls_entropy_context));
    if (!entropy) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_entropy_context));
        goto exit;
    }
    ctr_drbg = (mbedtls_ctr_drbg_context *)mbedtls_calloc(1, sizeof(mbedtls_ctr_drbg_context));
    if (!ctr_drbg) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ctr_drbg_context));
        goto exit;
    }
    ssl = (mbedtls_ssl_context *)mbedtls_calloc(1, sizeof(mbedtls_ssl_context));
    if (!ssl) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ssl_context));
        goto exit;
    }
    conf = (mbedtls_ssl_config *)mbedtls_calloc(1, sizeof(mbedtls_ssl_config));
    if (!conf) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_ssl_config));
        goto exit;
    }
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    cacert = (mbedtls_x509_crt *)mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (!cacert) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_x509_crt));
        goto exit;
    }
    srvcert = (mbedtls_x509_crt *)mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (!srvcert) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_x509_crt));
        goto exit;
    }
    srvcert2 = (mbedtls_x509_crt *)mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (!srvcert2) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(mbedtls_x509_crt));
        goto exit;
    }
#endif
    server_opt = (server_options *)mbedtls_calloc(1, sizeof(server_options));
    if (!server_opt) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(server_options));
        goto exit;
    }
    memset(server_opt, 0, sizeof(server_options));
    /*
     * Make sure memory references are valid in case we exit early.
     */
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_init(cacert);
    mbedtls_x509_crt_init(srvcert);
    mbedtls_pk_init(&pkey);
    mbedtls_x509_crt_init(srvcert2);
    mbedtls_pk_init(&pkey2);
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_SSL_ALPN)
    memset((void *)alpn_list, 0, sizeof(alpn_list));
#endif

    if (0 == strncmp("help", Parameter_List[0].String_Value, strlen(Parameter_List[0].String_Value)))
        goto usage;

    if (QAPI_OK != ssl_server_parse_paramters(server_opt, Parameter_Count, Parameter_List))
        goto usage;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(server_opt->debug_level);
#endif

    if (server_opt->response_size < DFL_RESPONSE_SIZE)
        buffer_size = DFL_RESPONSE_SIZE + 1;
    else
        buffer_size = server_opt->response_size + 1;

    buf = mbedtls_calloc(1, buffer_size);
    if (buf == NULL) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", buffer_size);
        goto exit;
    }

    stats = mbedtls_calloc(1, sizeof(ssl_stats));
    if (stats == NULL) {
        SSL_CLIENT_PRINTF("Fail to allocate %u bytes\n", sizeof(ssl_stats));
        goto exit;
    }
    memset(stats, 0, sizeof(ssl_stats));

    if (server_opt->force_ciphersuite[0] > 0) {
        const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
        ciphersuite_info = mbedtls_ssl_ciphersuite_from_id(server_opt->force_ciphersuite[0]);

        if (ciphersuite_info == NULL) {
            SSL_CLIENT_PRINTF("Fail to find ciphersuite_info from id: %d\n", server_opt->force_ciphersuite[0]);
            goto exit;
        }

        if (server_opt->max_version != -1 && ciphersuite_info->min_tls_version > server_opt->max_version) {
            SSL_CLIENT_PRINTF("forced ciphersuite not allowed with this protocol version\n");
            goto usage;
        }
        if (server_opt->min_version != -1 && ciphersuite_info->max_tls_version < server_opt->min_version) {
            SSL_CLIENT_PRINTF("forced ciphersuite not allowed with this protocol version\n");
            goto usage;
        }

        /* If the server selects a version that's not supported by
         * this suite, then there will be no common ciphersuite... */
        if (server_opt->max_version == -1 || server_opt->max_version > ciphersuite_info->max_tls_version) {
            server_opt->max_version = ciphersuite_info->max_tls_version;
        }
        if (server_opt->min_version < ciphersuite_info->min_tls_version) {
            server_opt->min_version = ciphersuite_info->min_tls_version;
        }
    }

#if defined(MBEDTLS_SSL_ALPN)
    if (server_opt->alpn_string != NULL) {
        p = (char *)server_opt->alpn_string;
        i = 0;

        /* Leave room for a final NULL in alpn_list */
        while (i < ALPN_LIST_SIZE - 1 && *p != '\0') {
            alpn_list[i++] = p;

            /* Terminate the current string and move on to next one */
            while (*p != ',' && *p != '\0')
                p++;
            if (*p == ',')
                *p++ = '\0';
        }
    }
#endif /* MBEDTLS_SSL_ALPN */

    stats->ssl_stream_id = ssl_get_unused_id();
    if (stats->ssl_stream_id == MAX_STREAM) {
        SSL_CLIENT_PRINTF("Reach the limited streams\n");
        goto exit;
    }
    new_session = 1;

    /*
     * 0. Initialize the RNG and the session data
     */
    quit_ssl = 0;

    SSL_CLIENT_PRINTF("\n	. Seeding the random number generator...");
    mbedtls_entropy_init(entropy);
    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy, (const unsigned char *)server_pers,
                                     strlen(server_pers))) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 1.1. Load the trusted CA
     */
    SSL_CLIENT_PRINTF("  . Loading the CA root certificate ...");

    for (i = 0; demo_test_cas[i] != NULL; i++) {
        ret = mbedtls_x509_crt_parse(cacert, (const unsigned char *)demo_test_cas[i], demo_test_cas_len[i]);
        if (ret != 0)
            break;
    }

    if (ret < 0) {
        SSL_CLIENT_PRINTF(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok \n");

    /*
     * 1.2. Load own certificate and private key
     *
     */
    SSL_CLIENT_PRINTF("  . Loading the server cert. and key...");

    ret = mbedtls_x509_crt_parse(srvcert, (const unsigned char *)demo_test_ca_crt_ec, sizeof(demo_test_ca_crt_ec));
    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        goto exit;
    }

    ret = mbedtls_x509_crt_parse(srvcert2, (const unsigned char *)demo_test_ca_crt_rsa, sizeof(demo_test_ca_crt_rsa));
    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_x509_crt_parse[2] returned -0x%x\n\n", -ret);
        goto exit;
    }

    ret = mbedtls_pk_parse_key(&pkey, (const unsigned char *)demo_test_ca_key_ec, sizeof(demo_test_ca_key_ec), NULL, 0,
                               mbedtls_ctr_drbg_random, ctr_drbg);
    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_pk_parse_key returned -0x%x\n\n", -ret);
        goto exit;
    }

    ret = mbedtls_pk_parse_key(&pkey2, (const unsigned char *)demo_test_ca_key_rsa, sizeof(demo_test_ca_key_rsa), NULL,
                               0, mbedtls_ctr_drbg_random, ctr_drbg);
    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n	!  mbedtls_pk_parse_key[2] returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    /*
     * 2. Setup the listening TCP socket
     */
    SSL_CLIENT_PRINTF("  . Bind on tcp://%s:%s/ ...", server_opt->server_addr ? server_opt->server_addr : "*",
                      server_opt->server_port);

    if ((ret = mbedtls_net_bind(&listen_fd, server_opt->server_addr, server_opt->server_port, MBEDTLS_NET_PROTO_TCP)) !=
        0) {
        SSL_CLIENT_PRINTF(" failed\n	! mbedtls_net_bind returned -0x%x\n\n", -ret);
        goto exit;
    }

    SSL_CLIENT_PRINTF(" ok\n");

    /*
     * 3. Setup stuff
     */
    SSL_CLIENT_PRINTF("  . Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret);
        goto exit;
    }

    if (server_opt->auth_mode != DFL_AUTH_MODE)
        mbedtls_ssl_conf_authmode(conf, server_opt->auth_mode);

    if (server_opt->cert_req_ca_list != DFL_CERT_REQ_CA_LIST)
        mbedtls_ssl_conf_cert_req_ca_list(conf, server_opt->cert_req_ca_list);

#if defined(MBEDTLS_SSL_ALPN)
    if (server_opt->alpn_string != NULL)
        if ((ret = mbedtls_ssl_conf_alpn_protocols(conf, alpn_list)) != 0) {
            SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_conf_alpn_protocols returned %d\n\n", ret);
            goto exit;
        }
#endif

    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
    mbedtls_ssl_conf_dbg(conf, demo_debug, NULL);

    if (server_opt->force_ciphersuite[0] != DFL_FORCE_CIPHER)
        mbedtls_ssl_conf_ciphersuites(conf, server_opt->force_ciphersuite);

#if defined(MBEDTLS_SSL_RENEGOTIATION)
    mbedtls_ssl_conf_renegotiation(conf, server_opt->renegotiation);

    if (server_opt->renego_delay != DFL_RENEGO_DELAY)
        mbedtls_ssl_conf_renegotiation_enforced(conf, server_opt->renego_delay);

    if (server_opt->renego_period != DFL_RENEGO_PERIOD) {
        PUT_UINT64_BE(renego_period, server_opt->renego_period, 0);
        mbedtls_ssl_conf_renegotiation_period(conf, renego_period);
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)

    mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);

    if ((ret = mbedtls_ssl_conf_own_cert(conf, srvcert, &pkey)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_conf_own_cert(conf, srvcert2, &pkey2)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    if (server_opt->min_version != DFL_MIN_VERSION)
        mbedtls_ssl_conf_min_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, server_opt->min_version);

    if (server_opt->max_version != DFL_MIN_VERSION)
        mbedtls_ssl_conf_max_version(conf, MBEDTLS_SSL_MAJOR_VERSION_3, server_opt->max_version);

    if ((ret = mbedtls_ssl_setup(ssl, conf)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    SSL_CLIENT_PRINTF(" ok\n");

reset:
    if (stats->exchanges) {
        end_time = hres_timer_curr_time_ms();
        stats->throughput = stats->total_bytes * 8 * 1000 / (end_time - begin_time);
        ssl_statistic_print_final(stats, end_time - begin_time);
    }

    if (ret == MBEDTLS_ERR_SSL_CLIENT_RECONNECT) {
        SSL_CLIENT_PRINTF("  ! Client initiated reconnection from same port\n");
        goto handshake;
    }

    stats->bytes = 0;
    stats->total_bytes = 0;
    stats->display_sec = 0;
    stats->exchanges = 0;

    mbedtls_net_free(&client_fd);

    mbedtls_ssl_session_reset(ssl);

    /*
     * 4. Wait until a client connects
     */
    SSL_CLIENT_PRINTF("  . Waiting for a remote connection ...");
    memset(&rd_set, 0, sizeof(fd_set));

    do {
        if (quit_ssl) {
            ret = 0;
            SSL_CLIENT_PRINTF(" Receive quit command\n");
            goto exit;
        }

        FD_SET(listen_fd.fd, &rd_set);
        tv.tv_sec = 2;
        if (select(listen_fd.fd + 1, &rd_set, NULL, NULL, &tv) > 0) {
            if ((ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL)) != 0) {
                SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_net_accept returned -0x%x\n\n", -ret);
                goto exit;
            } else
                break;
        }
    } while (1);

    mbedtls_ssl_conf_read_timeout(conf, 5000);

    SSL_CLIENT_PRINTF(" ok\n");

    /*
     * 5. Handshake
     */
handshake:
    SSL_CLIENT_PRINTF("  . Performing the SSL/TLS handshake...");

    begin_time = hres_timer_curr_time_ms();
    while ((ret = mbedtls_ssl_handshake(ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            break;
    }

    if (ret != 0) {
        SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
        goto reset;
    } else /* ret == 0 */
    {
        end_time = hres_timer_curr_time_ms();
        SSL_CLIENT_PRINTF(" ok\n  . SSL handshake consumes %d.%d seconds\n", (end_time - begin_time) / 1000,
                          (end_time - begin_time) % 1000);

        SSL_CLIENT_PRINTF("    [ Protocol is %s ]\n", mbedtls_ssl_get_version(ssl));

        const char *ciphersuite_info = mbedtls_ssl_get_ciphersuite(ssl);
        if (ciphersuite_info) {
            SSL_CLIENT_PRINTF("    [ Ciphersuite is %s ]\n", ciphersuite_info);
        } else {
            SSL_CLIENT_PRINTF("    [ Ciphersuite is NULL ]\n");
        }
    }

    if ((ret = mbedtls_ssl_get_record_expansion(ssl)) >= 0)
        SSL_CLIENT_PRINTF("    [ Record expansion is %d ]\n", ret);
    else
        SSL_CLIENT_PRINTF("    [ Record expansion is unknown (compression) ]\n");

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    SSL_CLIENT_PRINTF("    [ Maximum fragment length is %u ]\n", (unsigned int)mbedtls_ssl_get_max_frag_len(ssl));
#endif

#if defined(MBEDTLS_SSL_ALPN)
    if (server_opt->alpn_string != NULL) {
        const char *alp = mbedtls_ssl_get_alpn_protocol(ssl);
        SSL_CLIENT_PRINTF("    [ Application Layer Protocol is %s ]\n", alp ? alp : "(none)");
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 6. Verify the client certificate
     */
    SSL_CLIENT_PRINTF("  . Verifying peer X.509 certificate...");

    if ((flags = mbedtls_ssl_get_verify_result(ssl)) != 0) {
        SSL_CLIENT_PRINTF(" failed\n");
    } else
        SSL_CLIENT_PRINTF(" ok\n");

#endif /* MBEDTLS_X509_CRT_PARSE_C */

    if (server_opt->exchanges == 0) {
        goto close_notify;
    }

    stats->display_interval = (uint32_t)server_opt->display_interval;
    begin_time = hres_timer_curr_time_ms();
    ssl_display_last = hres_timer_curr_time_ms();
    ssl_display_next = ssl_display_last + 1000 * stats->display_interval;

data_exchange:
    /*
     * 7. Read the HTTP Request
     */
    if (quit_ssl) {
        ret = 0;
        SSL_CLIENT_PRINTF(" Receive quit command\n");
        goto exit;
    }

    if (server_opt->debug_level > 0)
        SSL_CLIENT_PRINTF("  < Read from client:");

    do {
        if (quit_ssl) {
            ret = 0;
            SSL_CLIENT_PRINTF(" Receive quit command\n");
            goto close_notify;
        }

        int terminated = 0;
        len = buffer_size - 1;
        memset(buf, 0, buffer_size);
        ret = mbedtls_ssl_read(ssl, buf, len);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_TIMEOUT)
            continue;

        if (ret <= 0) {
            switch (ret) {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    SSL_CLIENT_PRINTF(" connection was closed gracefully\n");
                    goto close_notify;

                case 0:
                case MBEDTLS_ERR_NET_CONN_RESET:
                    SSL_CLIENT_PRINTF(" connection was reset by peer\n");
                    ret = MBEDTLS_ERR_NET_CONN_RESET;
                    goto reset;

                default:
                    SSL_CLIENT_PRINTF(" mbedtls_ssl_read returned -0x%x\n", -ret);
                    goto reset;
            }
        }

        if (mbedtls_ssl_get_bytes_avail(ssl) == 0) {
            request_size = len = ret;
            buf[len] = '\0';
            if (server_opt->debug_level > 0)
                SSL_CLIENT_PRINTF(" %d bytes read\n\n%s\n", len, (char *)buf);

            /* End of message should be detected according to the syntax of the
             * application protocol (eg HTTP), just use a dummy test here. */
            if (buf[len - 1] == '\n')
                terminated = 1;
        } else {
            int extra_len, ori_len;
            unsigned char *larger_buf = 0, *tmp_buf;

            ori_len = ret;
            extra_len = (int)mbedtls_ssl_get_bytes_avail(ssl);
            request_size = ori_len + extra_len;

            larger_buf = mbedtls_calloc(1, ori_len + extra_len + 1);
            if (larger_buf == NULL) {
                SSL_CLIENT_PRINTF("  ! memory allocation failed\n");
                ret = 1;
                goto reset;
            }

            memset(larger_buf, 0, ori_len + extra_len);
            memscpy(larger_buf, ori_len + extra_len + 1, buf, ori_len);

            tmp_buf = buf;
            buf = larger_buf;
            buffer_size = ori_len + extra_len + 1;
            mbedtls_free(tmp_buf);

            /* This read should never fail and get the whole cached data */
            ret = mbedtls_ssl_read(ssl, larger_buf + ori_len, extra_len);
            if (ret != extra_len || mbedtls_ssl_get_bytes_avail(ssl) != 0) {
                SSL_CLIENT_PRINTF("  ! mbedtls_ssl_read failed on cached data\n");
                ret = 1;
                goto reset;
            }

            larger_buf[ori_len + extra_len] = '\0';
            if (server_opt->debug_level > 0)
                SSL_CLIENT_PRINTF(" %u bytes read (%u + %u)\n\n%s\n", ori_len + extra_len, ori_len, extra_len,
                                  (char *)larger_buf);

            /* End of message should be detected according to the syntax of the
             * application protocol (eg HTTP), just use a dummy test here. */
            if (larger_buf[ori_len + extra_len - 1] == '\n')
                terminated = 1;
        }

        if (terminated) {
            ret = 0;
            break;
        }
    } while (1);

    stats->bytes += request_size;
    stats->total_bytes += request_size;

    now = hres_timer_curr_time_ms();
    if ((stats->display_interval > 0) && (now >= ssl_display_next)) {
        stats->throughput = stats->bytes * 8 * 1000 / (now - ssl_display_last);
        ssl_statistic_print_period(stats);
        ssl_display_last = now;
        ssl_display_next = now + 1000 * stats->display_interval;
        stats->bytes = 0;
    }

    /*
     * 8a. Request renegotiation while client is waiting for input from us.
     * (only on the first exchange, to be able to test retransmission)
     */
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    if (server_opt->renegotiate && !stats->exchanges) {
        SSL_CLIENT_PRINTF("  . Requestion renegotiation...");

        while ((ret = mbedtls_ssl_renegotiate(ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_renegotiate returned %d\n\n", ret);
                goto reset;
            }
        }

        SSL_CLIENT_PRINTF(" ok\n");
    }
#endif /* MBEDTLS_SSL_RENEGOTIATION */

    /*
     * 8. Write the 200 Response
     */
    if (server_opt->rx_only) {
        stats->exchanges++;
        if (stats->exchanges != (uint32_t)server_opt->exchanges) {
            goto data_exchange;
        } else
            goto close_notify;
    }

    if (server_opt->debug_level > 0)
        SSL_CLIENT_PRINTF("  > Write to client:");

    const char *ciphersuite_info = mbedtls_ssl_get_ciphersuite(ssl);
    if (ciphersuite_info) {
        len = snprintf((char *)buf, buffer_size - 1, HTTP_RESPONSE, ciphersuite_info);
    } else {
        len = snprintf((char *)buf, buffer_size - 1, HTTP_RESPONSE, "");
    }

    /* Add padding to the response to reach opt.response_size in length */
    if (server_opt->response_size != DFL_RESPONSE_SIZE && len < server_opt->response_size) {
        memset(buf + len, 'B', server_opt->response_size - len);
        len += server_opt->response_size - len;
        buf[server_opt->response_size - 2] = '\r';
        buf[server_opt->response_size - 1] = '\n';
    }

    /* Truncate if response size is smaller than the "natural" size */
    if (server_opt->response_size != DFL_RESPONSE_SIZE && len > server_opt->response_size) {
        len = server_opt->response_size;

        /* Still end with \r\n unless that's really not possible */
        if (len >= 2)
            buf[len - 2] = '\r';
        if (len >= 1)
            buf[len - 1] = '\n';
    }

    for (written = 0, frags = 0; written < len; written += ret, frags++) {
        while ((ret = mbedtls_ssl_write(ssl, buf + written, len - written)) <= 0) {
            if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
                SSL_CLIENT_PRINTF(" failed\n  ! peer closed the connection\n\n");
                goto reset;
            }

            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                SSL_CLIENT_PRINTF(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);
                goto reset;
            }
        }
    }

    buf[written] = '\0';
    if (server_opt->debug_level > 0)
        SSL_CLIENT_PRINTF(" %d bytes written in %d fragments\n\n%s\n", written, frags, (char *)buf);
    ret = 0;

    /*
     * 8b. Continue doing data exchanges?
     */
    stats->exchanges++;
    if (stats->exchanges != (uint32_t)server_opt->exchanges) {
        goto data_exchange;
    }

    /*
     * 9. Done, cleanly close the connection
     */
close_notify:
    SSL_CLIENT_PRINTF("  . Closing the connection...");

    /* No error checking, the connection might be closed already */
    do
        ret = mbedtls_ssl_close_notify(ssl);
    while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
    ret = 0;

    SSL_CLIENT_PRINTF(" done\n");

    goto reset;

/*
 * Cleanup and exit
 */
exit:
    if (stats && (stats->exchanges)) {
        end_time = hres_timer_curr_time_ms();
        stats->throughput = stats->total_bytes * 8 * 1000 / (end_time - begin_time);
        ssl_statistic_print_final(stats, end_time - begin_time);
    }

    SSL_CLIENT_PRINTF("  . Cleaning up...");

    if (buf) {
        mbedtls_free(buf);
    }

    mbedtls_net_free(&client_fd);
    mbedtls_net_free(&listen_fd);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (cacert) {
        mbedtls_x509_crt_free(cacert);
        free(cacert);
    }
    if (srvcert) {
        mbedtls_x509_crt_free(srvcert);
        free(srvcert);
    }
    if (srvcert2) {
        mbedtls_x509_crt_free(srvcert2);
        free(srvcert2);
    }
    mbedtls_pk_free(&pkey);
    mbedtls_pk_free(&pkey2);
#endif

    if (ssl) {
        mbedtls_ssl_free(ssl);
        free(ssl);
    }
    if (conf) {
        mbedtls_ssl_config_free(conf);
        free(conf);
    }
    if (ctr_drbg) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        free(ctr_drbg);
    }
    if (entropy) {
        mbedtls_entropy_free(entropy);
        free(entropy);
    }

    if (new_session)
        ssl_stream_id[stats->ssl_stream_id] = 0;

    if (stats) {
        mbedtls_free(stats);
    }

    if (server_opt) {
        mbedtls_free(server_opt);
    }

    if (!ret)
        return QAPI_OK;
    else {
        SSL_CLIENT_PRINTF(" Function fail.\n");
        return QAPI_ERROR;
    }
usage:
    ssl_server_help();
    goto exit;
}

#endif
