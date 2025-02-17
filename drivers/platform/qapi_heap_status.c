/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_heap_status.h"
#include "qc_heap.h"
#include "lwip/stats.h"

qapi_Status_t qapi_Heap_Status(heap_status *hs)
{
    extern mem_heap_type amss_mem_heap;
    mem_heap_type *amss_mem_heap_ptr = &amss_mem_heap;

#if !CONFIG_MATTER_ENABLE
    extern struct stats_ lwip_stats;
    struct stats_ *lwip_stats_ptr = &lwip_stats;

    extern const struct memp_desc* const memp_pools[MEMP_MAX];
#endif

    if(!hs)
        return QAPI_ERR_INVALID_PARAM;

    hs->total_Bytes = amss_mem_heap_ptr->total_bytes;

    /* TODO: This is not the most accurate value since it does not account for
     * bytes spent on block padding and overheads. Need to improve this once
     * possible.
     */
    if(amss_mem_heap_ptr->used_bytes > amss_mem_heap_ptr->total_bytes)
    {
        /* Sanity check to avoid returning a negative number */
        return QAPI_ERROR;
    }
    hs->free_Bytes = amss_mem_heap_ptr->total_bytes - amss_mem_heap_ptr->used_bytes;
    hs->min_ever_free_bytes = (uint32_t) (amss_mem_heap_ptr->total_bytes - amss_mem_heap_ptr->max_used);

#if !CONFIG_MATTER_ENABLE
    /*lwip heap for tx*/
    hs->lwip_total_Bytes = lwip_stats_ptr->mem.avail;
    hs->lwip_free_Bytes = lwip_stats_ptr->mem.avail - lwip_stats_ptr->mem.used;
    hs->lwip_min_ever_free_bytes = lwip_stats_ptr->mem.avail - lwip_stats_ptr->mem.max;

    /*lwip pool num for rx*/
    hs->lwip_total_pool = memp_pools[MEMP_PBUF_POOL]->stats->avail*PBUF_POOL_BUFSIZE;
    hs->lwip_free_pool = memp_pools[MEMP_PBUF_POOL]->stats->avail*PBUF_POOL_BUFSIZE - memp_pools[MEMP_PBUF_POOL]->stats->used*PBUF_POOL_BUFSIZE;
    hs->lwip_min_ever_free_pool = memp_pools[MEMP_PBUF_POOL]->stats->avail*PBUF_POOL_BUFSIZE - memp_pools[MEMP_PBUF_POOL]->stats->max*PBUF_POOL_BUFSIZE;
#endif

    return QAPI_OK;
}
