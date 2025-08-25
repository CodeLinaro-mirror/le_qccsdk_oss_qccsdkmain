/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DPM_IP_H_
#define DPM_IP_H_

#include "stdint.h"

#define NT_IP_FRAME_TYPE   0x0800 /** IEEE EtherType for Internet protocol v4*/
#define NT_ARP_FRAME_TYPE  0x0806 /** IEEE EtherType for Address Resolution Protocol (ARP)*/
#define NT_IPV6_FRAME_TYPE 0x86DD /** IEEE EtherType for Internet Protocol v6*/

#define NT_IP_PROTO_ICMP   1
#define NT_IP_PROTO_UDP    17
#define NT_IP_PROTO_TCP    6
#define NT_IP6_PROTO_ICMP6 58

#define NT_MAC_ADDR_SIZE  6  /** MAC address size 6 Bytes. */
#define NT_IPV4_ADDR_SIZE 4  /** IPv4 address size 4 Bytes. */
#define NT_IPV6_ADDR_SIZE 16 /** IPv6 address size 16 Bytes. */

typedef struct ETH_HEADER {
    uint8_t xDestinationAddress[6]; /*  0 + 6 = 6  */
    uint8_t xSourceAddress[6];      /*  6 + 6 = 12 */
    uint16_t usFrameType;           /* 12 + 2 = 14 */
} __attribute__((packed)) ethernet_header_t;

typedef struct SNAP_HEADER {
    uint8_t xDestinationAddress[6]; /*  0 + 6 = 6  */
    uint8_t xSourceAddress[6];      /*  6 + 6 = 12 */
    uint16_t usFrameType;           /* 12 + 2 = 14 */
} __attribute__((packed)) snap_header_t;

typedef struct ARP_HEADER {
    uint16_t usHardwareType;            /*  0 +  2 =  2 */
    uint16_t usProtocolType;            /*  2 +  2 =  4 */
    uint8_t ucHardwareAddressLength;    /*  4 +  1 =  5 */
    uint8_t ucProtocolAddressLength;    /*  5 +  1 =  6 */
    uint16_t usOperation;               /*  6 +  2 =  8 */
    uint8_t xSenderHardwareAddress[6];  /*  8 +  6 = 14 */
    uint8_t ucSenderProtocolAddress[4]; /* 14 +  4 = 18  */
    uint8_t xTargetHardwareAddress[6];  /* 18 +  6 = 24  */
    uint32_t ulTargetProtocolAddress;   /* 24 +  4 = 28  */
} __attribute__((packed)) arp_header_t;

typedef struct IP_HEADER {
    uint8_t ucVersionHeaderLength;        /*  0 + 1 =  1 */
    uint8_t ucDifferentiatedServicesCode; /*  1 + 1 =  2 */
    uint16_t usLength;                    /*  2 + 2 =  4 */
    uint16_t usIdentification;            /*  4 + 2 =  6 */
    uint16_t usFragmentOffset;            /*  6 + 2 =  8 */
    uint8_t ucTimeToLive;                 /*  8 + 1 =  9 */
    uint8_t ucProtocol;                   /*  9 + 1 = 10 */
    uint16_t usHeaderChecksum;            /* 10 + 2 = 12 */
    uint32_t ulSourceIPAddress;           /* 12 + 4 = 16 */
    uint32_t ulDestinationIPAddress;      /* 16 + 4 = 20 */
} __attribute__((packed)) ip_header_t;

typedef struct IP6_HEADER {
    uint32_t ulVersionTCFlowLabel;
    uint16_t usPayloadLength;
    uint8_t ucNextHeader;
    uint8_t ucHopLimit;
    uint32_t ulSourceAddress[4];
    uint32_t ulDestinationAddress[4];
} __attribute__((packed)) ip6_header_t;

typedef struct IGMP_HEADER {
    uint8_t ucVersionType;     /* 0 + 1 = 1 */
    uint8_t ucMaxResponseTime; /* 1 + 1 = 2 */
    uint16_t usChecksum;       /* 2 + 2 = 4 */
    uint32_t usGroupAddress;   /* 4 + 4 = 8 */
} __attribute__((packed)) igmp_header_t;

typedef struct ICMP_HEADER {
    uint8_t ucTypeOfMessage;   /* 0 + 1 = 1 */
    uint8_t ucTypeOfService;   /* 1 + 1 = 2 */
    uint16_t usChecksum;       /* 2 + 2 = 4 */
    uint16_t usIdentifier;     /* 4 + 2 = 6 */
    uint16_t usSequenceNumber; /* 6 + 2 = 8 */
} __attribute__((packed)) icmp_header_t;

typedef struct ICMP6_HEADER {
    uint8_t ucTypeOfMessage;   /* 0 + 1 = 1 */
    uint8_t ucTypeOfService;   /* 1 + 1 = 2 */
    uint16_t usChecksum;       /* 2 + 2 = 4 */
    uint16_t usIdentifier;     /* 4 + 2 = 6 */
    uint16_t usSequenceNumber; /* 6 + 2 = 8 */
} __attribute__((packed)) icmp6_header_t;

typedef struct UDP_HEADER {
    uint16_t usSourcePort;      /* 0 + 2 = 2 */
    uint16_t usDestinationPort; /* 2 + 2 = 4 */
    uint16_t usLength;          /* 4 + 2 = 6 */
    uint16_t usChecksum;        /* 6 + 2 = 8 */
} __attribute__((packed)) udp_header_t;

typedef struct TCP_HEADER {
    uint16_t usSourcePort;      /* +  2 =  2 */
    uint16_t usDestinationPort; /* +  2 =  4 */
    uint32_t ulSequenceNumber;  /* +  4 =  8 */
    uint32_t ulAckNr;           /* +  4 = 12 */
    uint8_t ucTCPOffset;        /* +  1 = 13 */
    uint8_t ucTCPFlags;         /* +  1 = 14 */
    uint16_t usWindow;          /* +  2 = 15 */
    uint16_t usChecksum;        /* +  2 = 18 */
    uint16_t usUrgent;          /* +  2 = 20 */
} __attribute__((packed)) tcp_header_t;

typedef struct IP_PACKET {
    ethernet_header_t eth_hdr;
    ip_header_t ip_hdr;
} __attribute__((packed)) ip_packet_t;

typedef struct ARP_PACKET {
    ethernet_header_t eth_hdr; /*  0 + 14 = 14 */
    arp_header_t arp_hdr;      /* 14 + 28 = 42 */
} __attribute__((packed)) arp_packet_t;

typedef struct ip_to_mac_mapping {
    uint8_t mac_addr[6];
    uint8_t ip_addr[4];
} ip_to_mac_mapping_t;

#endif /* DPM_IP_H_ */
