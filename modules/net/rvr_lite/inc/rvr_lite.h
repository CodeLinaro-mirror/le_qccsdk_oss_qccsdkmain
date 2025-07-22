/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file rvr_lite.h
 * @brief struct, function declarations common to RVR Lite
 *========================================================================*/
#ifndef RVR_LITE__H
#define RVR_LITE__H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "sockets.h"
#include "timer.h"

#define CONFIG_SUPPOR_RVR_LITE
#ifdef CONFIG_SUPPOR_RVR_LITE
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define LO_BITS(x)     (uint32_t)((uint32_t)(x)&0xFFFFFFFFu)
#define HI_BITS(x)     (uint32_t)(((uint32_t)((x) >> 32)) & 0xFFFFFFFFu)
#define US_TO_TICKS(x) NT_MS_TO_TICKS(US_TO_MS(x))

#define htonll(x) ((((uint64_t)htonl(x)) << 32) | htonl((x) >> 32))
#define ntohll(x) ((((uint64_t)ntohl(x)) << 32) | ntohl((x) >> 32))

/* Enumeration for rvr_lite_tx thread signals */
enum {
    RVR_LITE_EVENT_TMR_EXPIRY,
    RVR_LITE_EVENT_TMR_CMD,
};

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/* RVR Lite Server Context */
typedef struct rvr_lite_server_ctx_s {
    uint8_t netif_id;
    uint8_t conn_state;  // 0-not connected yet 1-connected
    uint16_t local_port;
    uint16_t remote_port;
    ip_addr_t remote_ip;
    void *pcb;  // pcb depending on type
} rvr_lite_server_ctx_t;

/* RVR Lite Client Context */
typedef struct rvr_lite_client_ctx_s {
    uint8_t netif_id;
    uint8_t conn_state;  // 0-not connected yet 1-connected
    uint16_t local_port;
    uint16_t remote_port;
    ip_addr_t remote_ip;
    uint16_t nbytes;  // number to bytes in tx packet
    uint32_t duration;
    timer_type timer;
    uint32_t seq_no;
    uint64_t offset;
    uint64_t flow_id;
    int s_fd;  // socket descriptor
    int tos;   // type of service
    struct sockaddr_in rem_addr;
    task_info_type *task_info;  // task info required for hres timer
} rvr_lite_client_ctx_t;

typedef struct rvr_header_s {
    // Unique identifier for the flow to which this packet belongs
    uint64_t flow_id;
    // Timestamp from when the chunk was generated
    uint64_t gen_ts;
    // Last timestamp received on a given flow in the
    // opposite direction when the block was synthesized
    uint64_t rev_ts;
    // Timestamp at which the packet was sent
    uint64_t tx_ts;
    // Time between when the packet containing rev_ts is received and when it is included in
    // this packet
    uint32_t wait_time;
    // Seqno of the entire superchunk
    uint32_t superchunk_sn;
    // Seqno of the entire chunk
    uint64_t chunk_seqno;
    // total number of chunks in the superchunk
    uint16_t num_chunks_in_superchunk;
    // chunk type
    uint16_t chunk_type;
    // total num frags for a whole chunk
    uint16_t num_frags;
    // Seqno of this pkt (fragment within the chunk)
    uint16_t frag_id;
} __attribute__((packed)) rvr_header_t;

/*------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/
BaseType_t rvr_lite_start_req(ip_addr_t *remote_ip, uint16_t remote_port, uint16_t local_port, uint8_t netif_id,
                              uint16_t n_bytes, uint32_t duration, uint8_t tos, uint32_t flow_id, uint32_t offset);
void rvr_lite_stop_req(uint8_t netif_id);

qapi_Status_t rvr(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t rvr_stop(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

#endif  // CONFIG_SUPPOR_RVR_LITE
#endif  // RVR_LITE__H
