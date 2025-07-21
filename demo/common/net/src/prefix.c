/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file
 * Prefix sender module
 *
 */

#include "prefix.h"
#include "lwip/raw.h"
#include "lwip/prot/nd6.h"
#include "lwip/icmp.h"
#include <stdio.h>
#include "data_path.h"

#define PREFIX_OPTION_TYPE   3
#define PREFIX_OPTION_LENGTH 4
#define PREFIX_OPTION_FLAGS  0xC0

#define DEFAULT_HOP_LIMIT       64
#define DEFAULT_ROUTER_LIFETIME 1800

#ifndef ICMPV6_CHECKSUM_REQUIRED
#define ICMPV6_CHECKSUM_REQUIRED 1
#endif

#ifndef ICMPV6_CHECKSUM_POSITION_OFFSET
#define ICMPV6_CHECKSUM_POSITION_OFFSET 2
#endif

#if NET_SHELL_INFO
#define info_printf(msg, ...) printf(NET_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define info_printf(args...) \
    do {                     \
    } while (0)
#endif

static struct raw_pcb *prefix_pcb_v6;
static ip_addr_t target_addr;
static ip_addr_t prefix_addr;

static void prefix_raw_init(void)
{
#if LWIP_IPV6
    prefix_pcb_v6 = raw_new(IP6_NEXTH_ICMP6);

    LWIP_ASSERT("prefix_pcb_v6 != NULL", prefix_pcb_v6 != NULL);
    raw_bind(prefix_pcb_v6, IP_ANY_TYPE);
#endif
}

static void fill_ra_header(struct ra_header *p_ra)
{
    ICMPH_TYPE_SET(p_ra, ICMP6_TYPE_RA);
    ICMPH_CODE_SET(p_ra, 0);
    p_ra->chksum = 0;
    p_ra->current_hop_limit = DEFAULT_HOP_LIMIT;
    p_ra->flags = 0;
    p_ra->router_lifetime = lwip_htons(DEFAULT_ROUTER_LIFETIME);
    p_ra->reachable_time = 0;
    p_ra->retrans_timer = 0;
}

static void fill_prefix_option(struct prefix_option *prefix_opt, uint32_t prefixlen, uint32_t prefix_lifetime,
                               uint32_t valid_lifetime)
{
    prefix_opt->type = PREFIX_OPTION_TYPE;
    prefix_opt->length = PREFIX_OPTION_LENGTH;
    prefix_opt->prefix_length = prefixlen;
    prefix_opt->flags = PREFIX_OPTION_FLAGS;
    prefix_opt->valid_lifetime = lwip_htonl(valid_lifetime);
    prefix_opt->preferred_lifetime = lwip_htonl(prefix_lifetime);
    prefix_opt->reserved2[0] = 0;
    prefix_opt->reserved2[1] = 0;
    prefix_opt->reserved2[2] = 0;
    prefix_opt->site_prefix_length = 0;
    memcpy(&prefix_opt->prefix, &(prefix_addr.u_addr.ip6), sizeof(prefix_opt->prefix));
}

void prefix_send(ip_addr_t *ip_addr, uint32_t prefixlen, uint32_t prefix_lifetime, uint32_t valid_lifetime,
                 uint8_t netid)
{
    struct pbuf *p;
    struct ra_header *ra;
    struct prefix_option *prefix_opt;
    ip_addr_copy(prefix_addr, *ip_addr);

    prefix_raw_init();
    p = pbuf_alloc(PBUF_IP, sizeof(struct ra_header) + sizeof(struct prefix_option), PBUF_RAM);
    if (p == NULL) {
        printf("Error allocating pbuf\n");
        raw_remove(prefix_pcb_v6);
        return;
    }

    ra = (struct ra_header *)p->payload;
    fill_ra_header(ra);
    prefix_opt = (struct prefix_option *)(ra + 1);
    fill_prefix_option(prefix_opt, prefixlen, prefix_lifetime, valid_lifetime);

    ip6_addr_set_allnodes_linklocal(&(target_addr.u_addr.ip6));
    IP_SET_TYPE_VAL(target_addr, IPADDR_TYPE_V6);
    prefix_pcb_v6->chksum_reqd = ICMPV6_CHECKSUM_REQUIRED;
    prefix_pcb_v6->chksum_offset = ICMPV6_CHECKSUM_POSITION_OFFSET;
    prefix_pcb_v6->mcast_ifindex = netid;

    // send ICMPv6
    if (raw_sendto(prefix_pcb_v6, p, &target_addr) != ERR_OK) {
        info_printf("Error sending ICMPv6 RA message\n");
    } else {
        info_printf("ICMPv6 RA message sent successfully\n");
    }

    pbuf_free(p);
    raw_remove(prefix_pcb_v6);
    return;
}