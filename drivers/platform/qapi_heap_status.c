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

qapi_Status_t qapi_Heap_Status(heap_status *hs)
{
    extern mem_heap_type amss_mem_heap;
    mem_heap_type *amss_mem_heap_ptr = &amss_mem_heap;

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

    return QAPI_OK;
}
