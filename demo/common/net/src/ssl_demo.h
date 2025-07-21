/*
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

#ifndef _SSL_H_

#define _SSL_H_

/*
 * global options
 */
typedef struct client_options {
    const char *server_name;  /* hostname of the server (client only)     */
    const char *server_addr;  /* address of the server (client only)      */
    const char *server_port;  /* port on which the ssl service runs       */
    int debug_level;          /* level of debugging                       */
    int request_size;         /* pad request with header to requested size */
    const char *ca_file;      /* the file with the CA certificate(s)      */
    const char *crt_file;     /* the file with the client certificate     */
    const char *key_file;     /* the file with the client key             */
    int force_ciphersuite[2]; /* protocol/ciphersuite to use, or all      */
    int renegotiation;        /* enable / disable renegotiation           */
    int renegotiate;          /* attempt renegotiation?                   */
    int exchanges;            /* number of data exchanges                 */
    int min_version;          /* minimum protocol version accepted        */
    int max_version;          /* maximum protocol version accepted        */
    int auth_mode;            /* verify mode for connection               */
    const char *alpn_string;  /* ALPN supported protocols                 */
    int display_interval;     /* statistics display interval 				*/
    int tx_only;              /* no receive for client */
} client_options;

typedef struct server_options {
    const char *server_addr;  /* address on which the ssl service runs    */
    const char *server_port;  /* port on which the ssl service runs       */
    int debug_level;          /* level of debugging                       */
    int response_size;        /* pad response with header to requested size */
    int auth_mode;            /* verify mode for connection               */
    int cert_req_ca_list;     /* should we send the CA list?              */
    const char *ca_file;      /* the file with the CA certificate(s)      */
    const char *crt_file;     /* the file with the server certificate     */
    const char *key_file;     /* the file with the server key             */
    const char *crt_file2;    /* the file with the 2nd server certificate */
    const char *key_file2;    /* the file with the 2nd server key         */
    int renegotiation;        /* enable / disable renegotiation           */
    int renegotiate;          /* attempt renegotiation?                   */
    int renego_delay;         /* delay before enforcing renegotiation     */
    uint64_t renego_period;   /* period for automatic renegotiation       */
    const char *alpn_string;  /* ALPN supported protocols                 */
    int min_version;          /* minimum protocol version accepted        */
    int max_version;          /* maximum protocol version accepted        */
    int exchanges;            /* number of data exchanges                 */
    int force_ciphersuite[2]; /* protocol/ciphersuite to use, or all      */
    int display_interval;     /* statistics display interval 				*/
    int rx_only;              /* no send for server */
} server_options;

typedef struct ssl_stats {
    uint64_t bytes;
    uint64_t total_bytes;
    uint32_t exchanges;
    uint32_t display_interval;
    uint32_t display_sec;
    uint32_t ssl_stream_id;
    uint64_t throughput;
} ssl_stats;

qapi_Status_t ssl_quit(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t ssl_client(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t ssl_server(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

#endif
