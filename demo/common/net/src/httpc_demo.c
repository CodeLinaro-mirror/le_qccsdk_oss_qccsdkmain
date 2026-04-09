/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "qapi_status.h"
#include "net_shell.h"
#include "qapi_httpc.h"
#include "safeAPI.h"
#include "httpc_demo.h"
#include "mbedtls/ssl.h"
#include <unistd.h>
#include "fcntl.h"
#include "qcom_http_client.h"
#include "qat_api.h"

#ifdef CONFIG_QAT_OTA_DEMO
#include "ota_http.h"
#endif

#ifdef CONFIG_HTTP_CLIENT_DEMO

//#define HTTPC_DEMO_DEBUG
#ifdef HTTPC_DEMO_DEBUG
#pragma GCC push_options
#pragma GCC optimize("O0")
#endif

#define HTTPC_DEMO_MAX_NUM                (2)
#define HTTPC_DEMO_DEFAULT_MAX_BODY_LEN   2048
#define HTTPC_DEMO_MAX_BODY_LEN           10240
#define HTTPC_DEMO_DEFAULT_MAX_HEADER_LEN 350
#define MAX_PRINTF_LENGTH                 256

#define REQUEST_TIMEOUT_MS 5000 /* 5 sec */
#define BODY_BUFFER_SIZE   1010
#define HEADER_BUFFER_SIZE 300
#define RX_BUFFER_SIZE     1000
#define MAX_URL_LENGTH     256
#define MAX_HOST_LENGTH    64
#define MAX_CHUNK_SIZE     1000  // should <= BODY_BUFFER_SIZE
#define HEX_BYTES_PER_LINE 16

static uint8_t hd = 0;
static const char hexchar[] = "0123456789ABCDEF";

uint16_t httpc_demo_max_body_len = 0;
uint16_t httpc_demo_max_header_len = 0;
static uint8_t *data = NULL;

#ifdef CONFIG_QAT_HTTPC_DEMO
uint16_t at_httpc_method = 0;
uint16_t httpc_recvie_count = 0;
int at_rec_state = 0;
uint16_t at_rec_error_code = 0;
#endif

#ifdef CONFIG_QAT_OTA_DEMO
extern http_session_info_t *ota_http_sess;
extern void http_client_cb_ota(void *arg, int32_t state, void *http_resp);
#define HTTPC_PRINTF(...)                  \
    do { \
        http_session_info_t *sess = ota_http_sess; \
        if (!(sess && sess->status == HTTP_OTA_STATUS_RUNNING)) { \
            printf(__VA_ARGS__); \
        } \
    } while (0)
#else
#define HTTPC_PRINTF(...) printf(__VA_ARGS__)
#endif

struct http_client_demo_s {
    qapi_Net_HTTPc_handle_t client;
    uint32_t num;
    uint32_t total_len;
    qapi_Ssl_Config_t *sslCfg;
    qapi_Ssl_Cert_t *sslCert;
} http_client_demo[HTTPC_DEMO_MAX_NUM];

char default_ca_crt_rsa[] =
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
char default_ca_key_rsa[] =
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

char default_ca_crt_ec[] =
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

char default_ca_key_ec[] =
    "-----BEGIN EC PRIVATE KEY-----\r\n"
    "MHcCAQEEIBY2Zk3U/p8q3bcB/BW0pXxU/fxLDI2vxPYcg5zjqqmSoAoGCCqGSM49\r\n"
    "AwEHoUQDQgAEnS9V4KGlIZfU04RoMBla2cS6U1AYjr2NFRhlLQDaE35jc9bcbqKl\r\n"
    "XaeG8Ecazo90bi9UAOEUvHVR4fL66M288g==\r\n"
    "-----END EC PRIVATE KEY-----\r\n";

char default_cas[] =
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
    "-----END CERTIFICATE-----\r\n"
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

size_t default_cas_len = sizeof(default_cas);

/*****************************************************************************
 * FUNCTION: app_hexdump()
 *
 * Simple hexdump with optional ASCII
 *
 * PARAM2: void *           input buffer
 * PARAM3: unsigned         input buffer length
 * PARAM4: int              TRUE = include ASCII dump
 * PARAM5: int              TRUE = include [address]
 *
 * RETURNS: none
 *****************************************************************************/
void app_hexdump(void *inbuf, uint32_t inlen, int ascii, int addr)
{
    uint8_t *cp = (uint8_t *)inbuf;
    uint8_t *ap = (uint8_t *)inbuf;
    int len = (int)inlen;
    int clen, alen, i;
    char outbuf[96];
    char *outp = &outbuf[0];
    int line = 0;

    memset(outbuf, 0, sizeof(outbuf));
    while (len > 0) {
        if (addr)
            outp += snprintf(outp, sizeof(outbuf), "[%p] ", cp);

        clen = alen = min(HEX_BYTES_PER_LINE, len);

        /* display data in hex */
        for (i = 0; i < HEX_BYTES_PER_LINE; i++) {
            if (--clen >= 0) {
                uint8_t uc = *cp++;

                *outp++ = hexchar[(uc >> 4) & 0x0f];
                *outp++ = hexchar[(uc)&0x0f];
                *outp++ = ' ';
            } else if (line != 0) {
                *outp++ = ' ';
                *outp++ = ' ';
                *outp++ = ' ';
            }
        }

        if (ascii) {
            *outp++ = ' ';
            *outp++ = ' ';

            /* display data in ascii */
            while (--alen >= 0) {
                uint8_t uc = *ap++;

                *outp++ = ((uc >= 0x20) && (uc < 0x7f)) ? uc : '.';
            }
        }

        /* output the line */
        *outp++ = '\n';
        // print_line(outbuf, outp - &outbuf[0]);
        HTTPC_PRINTF("%s\n", outbuf);

        memset(outbuf, 0, sizeof(outbuf));
        outp = &outbuf[0];
        len -= HEX_BYTES_PER_LINE;
        line++;
    } /* while (len > 0) */
    return;
}

#define HEXDUMP(inbuf, inlen, ascii, addr) app_hexdump(inbuf, inlen, ascii, addr)

#ifdef CONFIG_QAT_HTTPC_DEMO
extern qbool_t conn_enable;
#endif
void http_client_cb_demo(void *arg, int32_t state, void *http_resp)
{
    (void)arg;
    qapi_Net_HTTPc_Response_t *temp = (qapi_Net_HTTPc_Response_t *)http_resp;
    struct http_client_demo_s *hc = (struct http_client_demo_s *)arg;
    uint32_t *ptotal_len = NULL;
    uint32_t tmp_len;
    uint32_t contentlength = 0;

    uint32_t print_len = MAX_PRINTF_LENGTH;
#ifdef CONFIG_QAT_HTTPC_DEMO
#define MAX_QAT_PRINTF_LENGTH 1050
    print_len = MAX_QAT_PRINTF_LENGTH - 10;
    char buffer[MAX_QAT_PRINTF_LENGTH];
#endif

    if (arg) {
        ptotal_len = &hc->total_len;
    } else {
        HTTPC_PRINTF("HTTP Client Demo arg error %d\n", state);
        return;
    }

    if (state >= QAPI_NET_HTTPC_RX_FINISHED) {
        int32_t resp_code = temp->resp_Code;

        if (temp->length && temp->data) {
            if (hd) {
                HTTPC_PRINTF("@@ Received %d bytes:\n", temp->length);
                HEXDUMP((char *)temp->data, temp->length, true, false);
            } else {
#ifdef CONFIG_QAT_HTTPC_DEMO
                // HTTPC_PRINTF("HTTP Client Demo state %d\n", state);
                if ((0 == httpc_recvie_count) &&
                    ((at_httpc_method == QAT_HTTP_GET) || (at_httpc_method == QAT_HTTP_HEAD) ||
                     (at_httpc_method == QAT_HTTP_POST))) {
                    snprintf(buffer, MAX_QAT_PRINTF_LENGTH, "+HTTPC: data:");
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
                }

                httpc_recvie_count++;
#endif
                if (data == NULL) {
                    if ((data = malloc(print_len + 1)) == NULL) {
                        HTTPC_PRINTF("HTTP Client Demo malloc error %d\n", state);
                        return;
                    }
                }

                /*print buffer is only 256B*/
                if (temp->length > print_len) {
                    tmp_len = temp->length;
                    while (tmp_len > print_len) {
                        // memcpy(data, temp->data, print_len);
                        memscpy(data, print_len + 1, temp->data, print_len);
                        data[print_len] = '\0';

#ifdef CONFIG_QAT_HTTPC_DEMO
                        if ((at_httpc_method == QAT_HTTP_GET) || (at_httpc_method == QAT_HTTP_HEAD) ||
                            (at_httpc_method == QAT_HTTP_POST)) {
                            memscpy(buffer, MAX_QAT_PRINTF_LENGTH, data, print_len);

                            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                            memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
                        }
#else
                        HTTPC_PRINTF("%s", data);
#endif

                        temp->data += print_len;
                        tmp_len -= print_len;
                    }
                    if (tmp_len > 0) {
                        // memcpy(data, temp->data, tmp_len);
                        memscpy(data, print_len + 1, temp->data, tmp_len);
                        data[tmp_len] = '\0';

#ifdef CONFIG_QAT_HTTPC_DEMO
                        if ((at_httpc_method == QAT_HTTP_GET) || (at_httpc_method == QAT_HTTP_HEAD) ||
                            (at_httpc_method == QAT_HTTP_POST)) {
                            memscpy(buffer, MAX_QAT_PRINTF_LENGTH, data, tmp_len);
                            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                            memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
                        }
#else
                        HTTPC_PRINTF("%s", data);
#endif
                    }
                } else {
                    memscpy(data, print_len + 1, temp->data, temp->length);
                    data[temp->length] = '\0';

#ifdef CONFIG_QAT_HTTPC_DEMO
                    if ((at_httpc_method == QAT_HTTP_GET) || (at_httpc_method == QAT_HTTP_HEAD) ||
                        (at_httpc_method == QAT_HTTP_POST)) {
                        memscpy(buffer, MAX_QAT_PRINTF_LENGTH, data, temp->length);
                        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                        memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
                    }
#else
                    HTTPC_PRINTF("%s", data);
#endif
                }
            }
            *ptotal_len += temp->length;
            contentlength = temp->contentlength;
        }

        if (state == QAPI_NET_HTTPC_RX_FINISHED) {
#ifdef CONFIG_QAT_HTTPC_DEMO

            if (at_httpc_method == QAT_HTTP_GETSIZE) {
                if ((resp_code >= 200) && (resp_code < 300)) {
                    snprintf(buffer, MAX_QAT_PRINTF_LENGTH, "+HTTPC: %d\r\n", contentlength);
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                } else {
                    snprintf(buffer, MAX_QAT_PRINTF_LENGTH, "+HTTPC: ERROR, Resp_code:%d\r\n", resp_code);
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                }

                memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
            } else if ((at_httpc_method == QAT_HTTP_GET) || (at_httpc_method == QAT_HTTP_HEAD) ||
                       (at_httpc_method == QAT_HTTP_POST)) {
                if ((resp_code >= 200) && (resp_code < 300)) {
                    snprintf(buffer, MAX_QAT_PRINTF_LENGTH, "+HTTPC: size:%d\r\n", *ptotal_len);
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                } else {
                    snprintf(buffer, MAX_QAT_PRINTF_LENGTH, "+HTTPC: ERROR, Resp_code:%d\r\n", resp_code);
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                }
                memset((void *)buffer, 0, MAX_QAT_PRINTF_LENGTH);
            }

            at_rec_error_code = resp_code;
            httpc_recvie_count = 0;
#endif
            HTTPC_PRINTF("=========> http client Received: total size %d, Resp_code %d\n", *ptotal_len, resp_code);
            *ptotal_len = 0;  // Finished
            if (data) {
                free(data);
                data = NULL;
            }

        } else if (state == QAPI_NET_HTTPC_RX_TUNNEL_ESTABLISHED) {
            HTTPC_PRINTF("#### TUNNEL ESTABLISHED: received %d bytes ####\n", *ptotal_len);
            *ptotal_len = 0;
#ifdef CONFIG_QAT_HTTPC_DEMO
            httpc_recvie_count = 0;
#endif
            if (data) {
                free(data);
                data = NULL;
            }
        } else if (state == QAPI_NET_HTTPC_RX_DATA_FROM_TUNNEL) {
            HTTPC_PRINTF("#### Received %d bytes from TUNNEL ####\n", *ptotal_len);
            *ptotal_len = 0;
#ifdef CONFIG_QAT_HTTPC_DEMO
            httpc_recvie_count = 0;
#endif
            if (data) {
                free(data);
                data = NULL;
            }

        } else if (state == QAPI_NET_HTTPC_RX_TUNNEL_CLOSED) {
            HTTPC_PRINTF("!!!! TUNNEL CLOSED !!!!\n");
            *ptotal_len = 0;
#ifdef CONFIG_QAT_HTTPC_DEMO
            httpc_recvie_count = 0;
#endif
            if (data) {
                free(data);
                data = NULL;
            }

        } else if (state == QAPI_NET_HTTPC_RX_CHUNK_CONTINUE) {
            HTTPC_PRINTF("!!!! CONTINUE RECV !!!!\n");
            *ptotal_len = 0;
#ifdef CONFIG_QAT_HTTPC_DEMO
            httpc_recvie_count = 0;
#endif
            if (data) {
                free(data);
                data = NULL;
            }
        }
    } else {
        if (QAPI_NET_HTTPC_RX_ERROR_SERVER_CLOSED == state) {
            HTTPC_PRINTF("HTTP Client server closed on client[%d].\n", hc->num);
#ifdef CONFIG_QAT_HTTPC_DEMO
            conn_enable = FALSE;
#endif
        } else
            HTTPC_PRINTF("HTTP Client Receive error: %d\nPlease input 'httpc disconnect %d'\n", state, hc->num);
        *ptotal_len = 0;
#ifdef CONFIG_QAT_HTTPC_DEMO
        httpc_recvie_count = 0;
#endif
        if (data) {
            free(data);
            data = NULL;
        }
    }

#ifdef CONFIG_QAT_HTTPC_DEMO
    at_rec_state = state;
    HTTPC_PRINTF("HTTP state: %d\n", state);

#endif
}

char *httpc_malloc_body_demo(uint32_t len)
{
    char *body = NULL;
    uint32_t i;

    body = malloc(len + 1);
    if (body) {
        for (i = 0; i < len; i++) {
            *(body + i) = 'A' + i % 26;
        }
        *(body + len) = '\0';
    } else {
        HTTPC_PRINTF("malloc failed\n");
    }

    return body;
}

qapi_Status_t ssl_config_default_value(qapi_Ssl_Config_t *config)
{
    if (!config)
        return QAPI_ERROR;

    config->protocol = MBEDTLS_SSL_MINOR_VERSION_3;
    config->alpn_string = NULL;
    config->force_ciphersuite[0] = 0;

    return QAPI_OK;
}

qapi_Status_t ssl_parse_config_parameters(qapi_Ssl_Config_t *config, uint32_t Parameter_Count,
                                          QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t argn;

    if (!config)
        return QAPI_ERROR;

    ssl_config_default_value(config);

    for (argn = 0; argn < Parameter_Count; argn++) {
        if (argn == Parameter_Count - 1) {
            HTTPC_PRINTF("What is value of %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        } else if (0 == strncmp("protocol", Parameter_List[argn].String_Value, 8)) {
            argn++;
            if (0 == strcmp("TLS1.2", Parameter_List[argn].String_Value)) {
                config->protocol = MBEDTLS_SSL_MINOR_VERSION_3;
            } else {
                HTTPC_PRINTF("ERROR: Invalid protocol %s?\n", Parameter_List[argn].String_Value);
            }
        } else if (0 == strncmp("cipher", Parameter_List[argn].String_Value, 17)) {
            argn++;
            config->force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id(Parameter_List[argn].String_Value);

            if (config->force_ciphersuite[0] == 0)
                return QAPI_ERROR;

            config->force_ciphersuite[1] = 0;
        } else if (0 == strncmp("alpn", Parameter_List[argn].String_Value, 4)) {
            argn++;

            config->alpn_string = Parameter_List[argn].String_Value;
        } else if (0 == strncmp("server_name", Parameter_List[argn].String_Value, 11)) {
            argn++;

            config->server_name = Parameter_List[argn].String_Value;
        } else {
            HTTPC_PRINTF("What is %s?\n", Parameter_List[argn].String_Value);
            return QAPI_ERROR;
        }
    }

    return QAPI_OK;
}

void ssl_free_config_parameters(qapi_Ssl_Config_t *config)
{
    if (config == NULL)
        return;

    if (config->alpn_string) {
        free(config->alpn_string);
        config->alpn_string = NULL;
    }
    if (config->server_name) {
        free(config->server_name);
        config->server_name = NULL;
    }

    free(config);
}

qapi_Status_t ssl_load_credential(uint8_t **cred, char *file, uint32_t *size)
{
    int fd, cred_len;

    fd = open(file, O_RDONLY, 0);
    if (fd == -1) {
        HTTPC_PRINTF("%s line %d: Open file %s failed.\n", __func__, __LINE__, file);
        return QAPI_ERROR;
    }

    cred_len = lseek(fd, 0, SEEK_END);
    if (cred_len <= 0) {
        close(fd);
        HTTPC_PRINTF("%s line %d: lseek failed.\n", __func__, __LINE__);
        return QAPI_ERROR;
    }

    if (*cred != NULL) {
        free(*cred);
        *cred = NULL;
    }

    *cred = (uint8_t *)malloc(cred_len + 1);
    if (*cred == NULL) {
        close(fd);
        HTTPC_PRINTF("%s line %d: allocation failure.\n", __func__, __LINE__);
        return QAPI_ERROR;
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, *cred, cred_len);
    close(fd);
    (*cred)[cred_len] = 0;

    *size = cred_len + 1;

    return QAPI_OK;
}

void ssl_free_credentials(qapi_Ssl_Cert_t *crt)
{
    if (crt == NULL)
        return;

    if (crt->pRootCa) {
        free(crt->pRootCa);
        crt->pRootCa = NULL;
        crt->rootCaSize = 0;
    }
    if (crt->pClientCert) {
        free(crt->pClientCert);
        crt->pClientCert = NULL;
        crt->clientCertSize = 0;
    }
    if (crt->pPrivateKey) {
        free(crt->pPrivateKey);
        crt->pPrivateKey = NULL;
        crt->privateKeySize = 0;
    }

    free(crt);
}

void httpc_command_help(void)
{
    HTTPC_PRINTF("httpc {start | stop}\n");
    HTTPC_PRINTF(
        "httpc new [-t <timeout_ms> -b <body_buffer_size> -h <header_buffer_size> -r <rx_buffer_size> -s -c "
        "<calist>]\n");
    HTTPC_PRINTF("httpc destroy <client_num>\n");
    HTTPC_PRINTF("httpc conn <client_num> <origin_server or proxy> [<port>]\n");
    HTTPC_PRINTF("httpc disconn <client_num>\n");
    HTTPC_PRINTF(
        "httpc {get | head | post | put | delete | patch} <client_num> [<url>] [<chunk_flag>] [<chunk_size>] "
        "[<total_size>]\n");
    HTTPC_PRINTF(" where <chunk_flag> <value> are:\n");
    HTTPC_PRINTF(
        "       chunk_flag 0x00|0x01|0x80|0x81 = 0x00: non chunk encoded without http header; 0x01: non chunk encoded "
        "with http header\n");
    HTTPC_PRINTF(
        "                                        0x80: chunk encoded without http header; 0x81: chunk encoded with "
        "http header\n");
    HTTPC_PRINTF("httpc tunnel <client_num> <origin_server> <port> [-s -c <calist>]\n");
    HTTPC_PRINTF("httpc sendraw <client_num> [<data>]\n");
    HTTPC_PRINTF("httpc setbody <client_num> [<len>]\n");
    HTTPC_PRINTF("httpc addheaderfield <client_num> <hdr_name> <hdr_value>\n");
    HTTPC_PRINTF("httpc clearheader <client_num>\n");
    HTTPC_PRINTF("httpc setparam <client_num> <key> <value>\n");
    HTTPC_PRINTF("httpc cbaddhead <client_num> {enable|disable}\n");

    // sslconfig_help("httpc sslconfig <client_num>");

    HTTPC_PRINTF("The following commands are deprecated:\n");
    HTTPC_PRINTF(" httpc connect <origin_server or proxy> <port> <timeout in ms>\n");
    HTTPC_PRINTF(" httpc disconnect <client_num>\n");
    HTTPC_PRINTF(" httpc config <httpc_demo_max_body_len> <httpc_demo_max_header_len>\n");
    HTTPC_PRINTF("\nExamples:\n");
    HTTPC_PRINTF(" httpc start\n");
    HTTPC_PRINTF(" httpc new         -or-\n");
    HTTPC_PRINTF(" httpc new -s -b 300 -h 200 -r 1024 -t 10000\n");

    HTTPC_PRINTF(" httpc sslconfig 1 protocol TLS1.2 cipher QAPI_NET_TLS_RSA_WITH_AES_256_GCM_SHA384\n");

    HTTPC_PRINTF(" httpc conn 1 www.example.com       -or-\n");
    HTTPC_PRINTF(" httpc conn 1 192.168.2.30 8080\n");
    HTTPC_PRINTF(" httpc get 1\n");
    HTTPC_PRINTF(" httpc get 1 /cgi/my_cgi_script.pl\n");
    HTTPC_PRINTF(" httpc tunnel 1 apis.google.com 443\n");
    HTTPC_PRINTF(" httpc sendraw 1 \"Hello, World!\"\n");
    HTTPC_PRINTF(" httpc disconn 1\n");
    HTTPC_PRINTF(" httpc destroy 1\n");
}

qapi_Status_t httpc_command_new_sess(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t i;
    int error = QAPI_OK;
    int timeout_ms = REQUEST_TIMEOUT_MS;
    int body_size = BODY_BUFFER_SIZE;
    int header_size = HEADER_BUFFER_SIZE;
    int rxbuffer_size = RX_BUFFER_SIZE;
    uint16_t ip_prefer = DEFAULT_IP_PREFER;
    uint16_t pre_alloc_ssl_buffer = DEFAULT_PRE_ALLOCATE_BUFFER;
    uint8_t https_auth_type = HTTPS_BOTH_AUTH;
    qbool_t secure_session = false;
    struct http_client_demo_s *arg = NULL;

    char *calist = NULL;
    char *cert_file = NULL;
    char *pri_key = NULL;
    qbool_t default_cert = false;

    for (i = 0; i < HTTPC_DEMO_MAX_NUM; i++) {
        if (http_client_demo[i].client == NULL) {
            arg = &http_client_demo[i];
            arg->num = i + 1;
            arg->total_len = 0;
            break;
        }
    }

    if (arg == NULL) {
        HTTPC_PRINTF("%s line %d: Cannot create more than %d clients\n", __func__, __LINE__, HTTPC_DEMO_MAX_NUM);
        return QAPI_ERROR;
    }

    for (i = 1; i < Parameter_Count; i++) {
        if (Parameter_List[i].String_Value[0] == '-') {
            switch (Parameter_List[i].String_Value[1]) {
                case 's': /* -s */
                    secure_session = true;
                    break;

                case 'a': /* -c ca_list */
                    i++;
                    calist = Parameter_List[i].String_Value;
                    if (arg->sslCert == NULL) {
                        arg->sslCert = malloc(sizeof(qapi_Ssl_Cert_t));
                        if (arg->sslCert == NULL)
                            return QAPI_ERROR;
                        memset(arg->sslCert, 0, sizeof(qapi_Ssl_Cert_t));
                    }

                    if (ssl_load_credential(&arg->sslCert->pRootCa, calist, &arg->sslCert->rootCaSize) != QAPI_OK) {
                        ssl_free_credentials(arg->sslCert);
                        arg->sslCert = NULL;

                        HTTPC_PRINTF("%s line %d: Fail to load Root CA: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    break;

                case 'c': /* -c cert */
                    i++;
                    cert_file = Parameter_List[i].String_Value;
                    if (arg->sslCert == NULL) {
                        arg->sslCert = malloc(sizeof(qapi_Ssl_Cert_t));
                        if (arg->sslCert == NULL)
                            return QAPI_ERROR;
                        memset(arg->sslCert, 0, sizeof(qapi_Ssl_Cert_t));
                    }
                    if (ssl_load_credential(&arg->sslCert->pClientCert, cert_file, &arg->sslCert->clientCertSize) !=
                        QAPI_OK) {
                        ssl_free_credentials(arg->sslCert);
                        arg->sslCert = NULL;
                        HTTPC_PRINTF("%s line %d: Fail to load client certificate: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }

                    break;

                case 'k': /* -k cert */
                    i++;
                    pri_key = Parameter_List[i].String_Value;
                    if (arg->sslCert == NULL) {
                        arg->sslCert = malloc(sizeof(qapi_Ssl_Cert_t));
                        if (arg->sslCert == NULL)
                            return QAPI_ERROR;
                        memset(arg->sslCert, 0, sizeof(qapi_Ssl_Cert_t));
                    }
                    if (ssl_load_credential(&arg->sslCert->pPrivateKey, pri_key, &arg->sslCert->privateKeySize) !=
                        QAPI_OK) {
                        ssl_free_credentials(arg->sslCert);
                        arg->sslCert = NULL;
                        HTTPC_PRINTF("%s line %d: Fail to load private key: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }

                    break;

                case 't': /* -t 10000 */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid timeout: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    timeout_ms = Parameter_List[i].Integer_Value;
                    break;

                case 'b': /* -b 300 */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid body size: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    body_size = Parameter_List[i].Integer_Value;
                    break;

                case 'h': /* -h 200 */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid header size: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    header_size = Parameter_List[i].Integer_Value;
                    break;

                case 'r': /* -r 512 */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid rxbuffer size: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    rxbuffer_size = (Parameter_List[i].Integer_Value < RX_BUFFER_SIZE)
                                        ? RX_BUFFER_SIZE
                                        : Parameter_List[i].Integer_Value;
                    break;
                case 'v': /* -v v4*/
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid || Parameter_List[i].Integer_Value < IP_V4 ||
                        Parameter_List[i].Integer_Value > IP_V6) {
                        HTTPC_PRINTF("%s line %d: Invalid ip type: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }

                    ip_prefer = Parameter_List[i].Integer_Value;
                    break;
                case 'p': /* -p pre allocate buffer */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid value of pre allocate buffer: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    pre_alloc_ssl_buffer = Parameter_List[i].Integer_Value;
                    break;
                case 'u': /* -u auth type */
                    i++;
                    if (!Parameter_List[i].Integer_Is_Valid) {
                        HTTPC_PRINTF("%s line %d: Invalid value of auth type: %s\n", __func__, __LINE__,
                                     Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                    }
                    https_auth_type = Parameter_List[i].Integer_Value;
                    break;
                default:
                    HTTPC_PRINTF("%s line %d: Unknown option: %s\n", __func__, __LINE__,
                                 Parameter_List[i].String_Value);
                    return QAPI_ERROR;
            }
        } else {
            HTTPC_PRINTF("%s line %d: Unknown option: %s\n", Parameter_List[i].String_Value);
            return QAPI_ERROR;
        }

        if (i == Parameter_Count) {
            HTTPC_PRINTF("%s line %d: What is value of %s?\n", __func__, __LINE__, Parameter_List[i - 1].String_Value);
            return QAPI_ERROR;
        }
    } /* for */
 
#ifdef CONFIG_QAT_OTA_DEMO
    if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
        arg->client = qapi_Net_HTTPc_New_sess2(timeout_ms, (uint32_t)secure_session, http_client_cb_ota, (void *)arg,
                                               body_size, header_size, rxbuffer_size, ip_prefer, pre_alloc_ssl_buffer, https_auth_type);
    }

    else
#endif
    {
        arg->client = qapi_Net_HTTPc_New_sess2(timeout_ms, (uint32_t)secure_session, http_client_cb_demo, (void *)arg,
                                               body_size, header_size, rxbuffer_size, ip_prefer, pre_alloc_ssl_buffer, https_auth_type);
    }

    if (arg->client == NULL) {
        HTTPC_PRINTF("%s line %d: Failed to create a client instance\n", __func__, __LINE__);
        if (secure_session) {
            if (!default_cert) {
                if (arg->sslCert) {
                    ssl_free_credentials(arg->sslCert);
                    arg->sslCert = NULL;
                }
            }

            if (arg->sslCfg) {
                ssl_free_config_parameters(arg->sslCfg);
                arg->sslCfg = NULL;
            }

            memset(arg, 0, sizeof(arg));
        }
        return QAPI_ERROR;
    } else {
        if (secure_session && https_auth_type != HTTPS_NOT_AUTH) {
            if (arg->sslCert == NULL) {
                // using default cert
                arg->sslCert = malloc(sizeof(qapi_Ssl_Cert_t));
                if (arg->sslCert == NULL)
                    return QAPI_ERR_NO_MEMORY;

                arg->sslCert->pRootCa = (uint8_t *)default_cas;
                arg->sslCert->rootCaSize = default_cas_len;
                arg->sslCert->pClientCert = (uint8_t *)default_ca_crt_ec;
                arg->sslCert->clientCertSize = sizeof(default_ca_crt_ec);
                arg->sslCert->pPrivateKey = (uint8_t *)default_ca_key_ec;
                arg->sslCert->privateKeySize = sizeof(default_ca_key_ec);

                default_cert = true;

                HTTPC_PRINTF(
                    "%s line %d: Using preload cert, CA root size:%d, client cert size:%d, private key size:%d\n",
                    __func__, __LINE__, arg->sslCert->rootCaSize, arg->sslCert->clientCertSize,
                    arg->sslCert->privateKeySize);
            } else {
                if (!arg->sslCert->pRootCa) {
                    ssl_free_credentials(arg->sslCert);
                    arg->sslCert = NULL;
                    HTTPC_PRINTF("%s line %d: CA Root is empty.\n", __func__, __LINE__);

                    return QAPI_ERROR;
                }
            }

            error = qapi_Net_HTTPc_Configure_Cert(arg->client, arg->sslCert);
            if (error != QAPI_OK && !default_cert) {
                ssl_free_credentials(arg->sslCert);
                arg->sslCert = NULL;

                return QAPI_ERROR;
            }
        }
    }

    HTTPC_PRINTF("%s line %d: HTTP client created. <client num> = %d\n", __func__, __LINE__, arg->num);
    HTTPC_PRINTF("%s line %d: %ssecure  rxbuf:%d  bodybuf:%d  headerbuf:%d  timeout:%dms\n", __func__, __LINE__,
                 secure_session ? "" : "in", rxbuffer_size, body_size, header_size, timeout_ms);

    return QAPI_OK;
}

qapi_Status_t httpc_command_conn(struct http_client_demo_s *arg, uint32_t Parameter_Count,
                                 QAPI_Console_Parameter_t *Parameter_List)
{
    int error = QAPI_OK;
    char *host;
    uint16_t port = 0;
    uint32_t server_offset = 0;

    if (Parameter_Count < 3) {
        httpc_command_help();
        return QAPI_ERROR;
    }

    host = Parameter_List[2].String_Value;
    if (strlen(host) > MAX_HOST_LENGTH) {
        HTTPC_PRINTF("%s line %d: Hostname too long. Cannot be over %d\n", __func__, __LINE__, MAX_HOST_LENGTH);
        return QAPI_ERROR;
    }

    /* httpc connect https://www.example.com 443 36000 */
    if (strncmp(Parameter_List[2].String_Value, "https://", 8) == 0) {
        server_offset = 8;
    }

    /* httpc connect http://www.example.com 80 36000 */
    else if (strncmp(Parameter_List[2].String_Value, "http://", 7) == 0) {
        server_offset = 7;
    }

    /* httpc connect www.example.com 80 36000 */
    else {
        server_offset = 0;
    }

    if (Parameter_Count >= 4) {
        if (!Parameter_List[3].Integer_Is_Valid) {
            HTTPC_PRINTF("%s line %d: Invalid port\n", __func__, __LINE__);
            return QAPI_ERROR;
        }
        port = Parameter_List[3].Integer_Value;
    }

    if (arg->sslCfg != NULL) /* SSL parameters are parsed */
    {
        error = qapi_Net_HTTPc_Configure_SSL(arg->client, arg->sslCfg);
    }

    if (error == QAPI_OK) {
        error = qapi_Net_HTTPc_Connect(arg->client, (const char *)(host + server_offset), port);
    }

    if (error) {
        HTTPC_PRINTF("%s line %d: conn failed %d\n", __func__, __LINE__, error);

        qapi_Net_HTTPc_Free_sess(arg->client);

        if (arg->sslCfg) {
            ssl_free_config_parameters(arg->sslCfg);
            arg->sslCfg = NULL;
        }
        if (arg->sslCert) {
            if (arg->sslCert->pRootCa != (uint8_t *)default_cas) {
                ssl_free_credentials(arg->sslCert);
                arg->sslCert = NULL;
            }

            else
                HTTPC_PRINTF("%s line %d: using default cert, won't free.\n", __func__, __LINE__);
        }

        memset(arg, 0, sizeof(*arg));
        return QAPI_ERROR;
    }

    HTTPC_PRINTF("%s line %d: conn to %s:%d succeeded\n", __func__, __LINE__, host, port);
    return QAPI_OK;
}

qapi_Status_t httpc_command_sslconfig(struct http_client_demo_s *arg, uint32_t Parameter_Count,
                                      QAPI_Console_Parameter_t *Parameter_List)
{
    int error = QAPI_OK;

    if (Parameter_Count < 3) {
        HTTPC_PRINTF("What are SSL parameters?\n");
        return QAPI_OK;
    }

    if (arg->sslCfg == NULL) {
        arg->sslCfg = malloc(sizeof(qapi_Ssl_Config_t));
        if (arg->sslCfg == NULL) {
            HTTPC_PRINTF("Allocation failure for ssl configuration\n");
            return QAPI_ERROR;
        }
        memset(arg->sslCfg, 0, sizeof(qapi_Ssl_Config_t));
    } else {
        /* free previous ssl parameters */
        ssl_free_config_parameters(arg->sslCfg);
        arg->sslCfg = NULL;
    }

    /* Parse SSL config parameters from command line */
    error = ssl_parse_config_parameters(arg->sslCfg, Parameter_Count - 2, &Parameter_List[2]);
    if (error != QAPI_OK) {
        httpc_command_help();
        return QAPI_ERROR;
    }

    return QAPI_OK;
}

qapi_Status_t httpc_command_handler(uint32_t __attribute__((__unused__)) Parameter_Count,
                                    QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    int error = QAPI_OK;
    struct http_client_demo_s *arg = NULL;
    char *command = Parameter_List[0].String_Value;
    uint32_t num = 0;
    qapi_Net_HTTPc_Method_e req_cmd = 0;

    if (Parameter_Count < 1) {
    usage:
        httpc_command_help();
        return QAPI_ERROR;
    }

    if (strcmp(command, "start") == 0) {
        HTTPC_PRINTF("HTTP Client start \r\n");
        error = qapi_Net_HTTPc_Start();
        if (error) {
            HTTPC_PRINTF("HTTP Client start failed %d\r\n", error);
            return QAPI_ERROR;
        }
        return QAPI_OK;
    }
    if (strcmp(command, "stop") == 0) {
        uint32_t i;
        error = qapi_Net_HTTPc_Stop();
        for (i = 0; i < HTTPC_DEMO_MAX_NUM; i++) {
            arg = &http_client_demo[i];

            if (arg->sslCfg) {
                ssl_free_config_parameters(arg->sslCfg);
                arg->sslCfg = NULL;
            }
            memset(arg, 0, sizeof(struct http_client_demo_s));
        }
        if (error) {
            HTTPC_PRINTF("HTTP Client stop failed %d\r\n", error);
            return QAPI_ERROR;
        }
        return QAPI_OK;
    }
    if (strcmp(command, "config") == 0) {
        if (Parameter_Count < 3) {
            goto usage;
        }
        if (Parameter_List[1].Integer_Value % 4 != 0)
            goto usage;

        httpc_demo_max_body_len = (Parameter_List[1].Integer_Value > HTTPC_DEMO_MAX_BODY_LEN)
                                      ? HTTPC_DEMO_MAX_BODY_LEN
                                      : Parameter_List[1].Integer_Value;
        httpc_demo_max_header_len = (Parameter_List[2].Integer_Value > HTTPC_DEMO_DEFAULT_MAX_HEADER_LEN)
                                        ? HTTPC_DEMO_DEFAULT_MAX_HEADER_LEN
                                        : Parameter_List[2].Integer_Value;
        HTTPC_PRINTF("Max body len:%d max header len:%d\r\n", httpc_demo_max_body_len, httpc_demo_max_header_len);
        return QAPI_OK;
    }
    if (strcmp(command, "hd") == 0) {
        hd ^= 1;
        return QAPI_OK;
    }

    /*       [0] [1] [2]      [3]  [4]      [5]  [6]         [7]  [8]            [9]
     * httpc new -t <timeout> -b <body_len> -h  <header_len> -r  <rxbuffer_size> -s
     */
    if (strcmp(command, "new") == 0) {
        return httpc_command_new_sess(Parameter_Count, Parameter_List);
    }

    /*************************************************************************
     * For commands which require <client_num>
     * <client_num> should be in between 1 and HTTPC_DEMO_MAX_NUM (inclusive)
     *************************************************************************/
    if (Parameter_Count < 2) {
        HTTPC_PRINTF("What is client_num?\n");
        return QAPI_OK;
    }

    num = Parameter_List[1].Integer_Value;
    if (num <= 0 || num > HTTPC_DEMO_MAX_NUM) {
        HTTPC_PRINTF("Client_num must be in between 1 and %d\n", HTTPC_DEMO_MAX_NUM);
        return QAPI_ERROR;
    }

    arg = &http_client_demo[num - 1];
    if (arg->client == NULL || arg->num != num) {
        HTTPC_PRINTF("Client %d does not exist\n", num);
        return QAPI_ERROR;
    }

    /*       [0]        [1]         [2]      [3]    [4]  [5] [6]   [7]
     * httpc sslconfig <client_num> protocol TLS1.2 time 1   alert 0
     */
    if (strncmp(command, "sslconfig", 3) == 0) {
        return httpc_command_sslconfig(arg, Parameter_Count, Parameter_List);
    }

    /*       [0]  [1]           [2]               [3]
     * httpc conn <client num> <server or proxy> [<port>]
     */
    if (strcmp(command, "conn") == 0) {
        return httpc_command_conn(arg, Parameter_Count, Parameter_List);
    }

    if (strcmp(command, "disconnect") == 0 || /* deprecated */
        strcmp(command, "destroy") == 0) {
        qapi_Net_HTTPc_Free_sess(arg->client);

        if (arg->sslCfg) {
            ssl_free_config_parameters(arg->sslCfg);
            arg->sslCfg = NULL;
        }
        if (arg->sslCert) {
            if (arg->sslCert->pRootCa != (uint8_t *)default_cas) {
                ssl_free_credentials(arg->sslCert);
                arg->sslCert = NULL;
            } else {
                HTTPC_PRINTF("using default cert, won't free.\n");
            }
        }

        memset(arg, 0, sizeof(struct http_client_demo_s));

        return QAPI_OK;
    }

    if (strncmp(command, "disconn", 4) == 0) {
        qapi_Net_HTTPc_Disconnect(arg->client);
        return QAPI_OK;
    }

    if (strcmp(command, "get") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_GET_E;
    } else if (strcmp(command, "put") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_PUT_E;
    } else if (strcmp(command, "post") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_POST_E;
    } else if (strcmp(command, "patch") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_PATCH_E;
    } else if (strcmp(command, "head") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_HEAD_E;
    } else if (strcmp(command, "delete") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_DELETE_E;
    } else if (strcmp(command, "tunnel") == 0) {
        req_cmd = QAPI_NET_HTTP_CLIENT_CONNECT_E;
    }

    /*       [0]    [1]           [2]            [3]    [4] [5] [6]
     * httpc tunnel <client_num> <origin server> <port> [-s -c  <calist>]
     */

    if (req_cmd == QAPI_NET_HTTP_CLIENT_CONNECT_E) {
#ifdef HTTP_TUNNEL
        ip6_addr addr6;
        char host_port_string[80];
        int i;
        qbool_t origin_server_is_https = false; /* origin server is NOT an HTTPS server */
        char *calist = NULL;

        if (Parameter_Count < 4) {
            HTTPC_PRINTF("Missing <origin server> and/or <port>\n");
            return QAPI_ERROR;
        }

        for (i = 4; i < Parameter_Count; i++) {
            if (Parameter_List[i].String_Value[0] == '-') {
                switch (Parameter_List[i].String_Value[1]) {
                    case 's': /* -s */
                        origin_server_is_https = true;
                        break;

                    case 'c': /* -c ca_list.bin */
                        i++;
                        calist = Parameter_List[i].String_Value;
                        break;

                    default:
                        HTTPC_PRINTF("Unknown option: %s\n", Parameter_List[i].String_Value);
                        return QAPI_ERROR;
                }
            } else {
                HTTPC_PRINTF("Unknown option: %s\n", Parameter_List[i].String_Value);
                return QAPI_ERROR;
            }

            if (i == Parameter_Count) {
                HTTPC_PRINTF("What is value of %s?\n", Parameter_List[i - 1].String_Value);
                return QAPI_ERROR;
            }
        } /* for */

        if (inet_pton(AF_INET6, Parameter_List[2].String_Value, &addr6) == 0) /* an IPv6 address */
        {
            snprintf(host_port_string, sizeof(host_port_string), "[%s]:%s", Parameter_List[2].String_Value,
                     Parameter_List[3].String_Value);
        } else {
            snprintf(host_port_string, sizeof(host_port_string), "%s:%s", Parameter_List[2].String_Value,
                     Parameter_List[3].String_Value);
        }

        if (Parameter_List[3].Integer_Value == 443) {
            origin_server_is_https = true;
        }

        if (origin_server_is_https) {
            error = qapi_Net_HTTPc_Tunnel_To_HTTPS(arg->client, (const char *)calist, (const char *)host_port_string);
        } else {
            error = qapi_Net_HTTPc_Request(arg->client, req_cmd, (const char *)host_port_string);
        }
#else
        HTTPC_PRINTF("HTTP tunnel is not supported yet\n");
        return QAPI_ERROR;
#endif
    } else if (Parameter_Count == 6 && req_cmd) {
        char *chunk = NULL;
        char *url = Parameter_List[2].String_Value;
        uint8_t chunk_flag = Parameter_List[3].Integer_Value;
        uint32_t chunk_size = Parameter_List[4].Integer_Value;
        int32_t total_size = Parameter_List[5].Integer_Value;

        if (strlen(url) > MAX_URL_LENGTH) {
            HTTPC_PRINTF("URL too long. Cannot be over %d\n", MAX_URL_LENGTH);
            return QAPI_ERROR;
        }

        if (chunk_size > MAX_CHUNK_SIZE) {
            HTTPC_PRINTF("Chunk size %d, it's too long. Cannot be over %d\n", chunk_size, MAX_CHUNK_SIZE);
            return QAPI_ERROR;
        }
#ifndef CONFIG_QAT_HTTPC_DEMO
        if (chunk_size > 0) {
            chunk = httpc_malloc_body_demo(chunk_size);
        }
#endif
        error = qapi_Net_HTTPc_Send_Chunk(arg->client, req_cmd, (const char *)url, chunk, chunk_size, chunk_flag,
                                          total_size);
        if (chunk != NULL) {
            free(chunk);
        }
    }
    /* httpc get <client_num> [<url>] */
    else if (req_cmd) {
        char *url = "/";

        if (Parameter_Count >= 3) {
            url = Parameter_List[2].String_Value;
            if (strlen(url) > MAX_URL_LENGTH) {
                HTTPC_PRINTF("URL too long. Cannot be over %d\n", MAX_URL_LENGTH);
                return QAPI_ERROR;
            }
        }
        error = qapi_Net_HTTPc_Request(arg->client, req_cmd, (const char *)url);
    }
    /*       [0]      [1]         [2]     [3]
     * httpc setbody <client_num> [<len>] [<data>]
     */
    else if (strcmp(command, "setbody") == 0) {
        char *body = NULL;
        uint32_t len = BODY_BUFFER_SIZE;

        if (Parameter_Count > 2) {
            len = Parameter_List[2].Integer_Value;
        }

#ifdef CONFIG_QAT_HTTPC_DEMO
        if (Parameter_Count > 2) {
            body = Parameter_List[3].String_Value;
        }
        if (!body)
            return QAPI_ERROR;

        uint32_t bodylen = strlen(body);
        if (bodylen != len) {
            HTTPC_PRINTF("warning: body len:%d,expected len:%d\n", bodylen, len);
            if (bodylen < len) {
                len = bodylen;
            }
        }

        HTTPC_PRINTF("send data len:%d\n", len);
        error = qapi_Net_HTTPc_Set_Body(arg->client, (const char *)body, len);
        body = NULL;
#else
        if (len > BODY_BUFFER_SIZE)
            len = BODY_BUFFER_SIZE;

        body = httpc_malloc_body_demo(len);

        if (!body)
            return QAPI_ERROR;

        HTTPC_PRINTF("body len = %d\n", len);

        error = qapi_Net_HTTPc_Set_Body(arg->client, (const char *)body, len);
        free(body);
#endif
    } else if (strcmp(command, "addheaderfield") == 0) {
        if (Parameter_Count < 4) {
            HTTPC_PRINTF("Missing parameters\n");
            return QAPI_ERROR;
        }
        error = qapi_Net_HTTPc_Add_Header_Field(arg->client, Parameter_List[2].String_Value,
                                                Parameter_List[3].String_Value);
        HTTPC_PRINTF("addheader,name:%s,val:%s\n", Parameter_List[2].String_Value, Parameter_List[3].String_Value);
    } else if (strcmp(command, "clearheader") == 0) {
        error = qapi_Net_HTTPc_Clear_Header(arg->client);
    } else if (strcmp(command, "setparam") == 0) {
        if (Parameter_Count < 4) {
            HTTPC_PRINTF("Missing parameters\n");
            return QAPI_ERROR;
        }
        error = qapi_Net_HTTPc_Set_Param(arg->client, Parameter_List[2].String_Value, Parameter_List[3].String_Value);
    } else if (strcmp(command, "cbaddhead") == 0) {
        uint16_t enable = 0;

        if (Parameter_Count < 3) {
            HTTPC_PRINTF("Missing parameters\n");
            return QAPI_ERROR;
        }

        if (strncmp(Parameter_List[2].String_Value, "enable", 3) == 0) {
            enable = 1;
        }
        error = qapi_Net_HTTPc_CB_Enable_Adding_Header(arg->client, enable);
    } else if (strncmp(command, "sendraw", 4) == 0) {
        char *buf;
        /*       [0]     [1]   [2]
         * httpc sendraw  1   ["<data string>"]
         */
        if (Parameter_Count < 3) {
            buf =
                "HEAD / HTTP/1.1\r\n"
                "Host: 172.217.14.110:443\r\n"
                "Accept: text/html, */*\r\n"
                "User-Agent: Quartz IOE\r\n"
                "Connection: keep-alive\r\n"
                "Cache-control: no-cache\r\n"
                "\r\n";
        } else {
            buf = Parameter_List[2].String_Value;
        }
        error = qapi_Net_HTTPc_Send_Data(arg->client, buf, strlen(buf));
    } else {
        HTTPC_PRINTF("Unknown http client command.\n");
        return QAPI_ERROR;
    }

    if (error) {
        HTTPC_PRINTF("http client %s failed on error: %d\n", command, error);
    }

    return QAPI_OK;
}
#endif

#ifdef HTTPC_DEMO_DEBUG
#pragma GCC pop_options
#endif
