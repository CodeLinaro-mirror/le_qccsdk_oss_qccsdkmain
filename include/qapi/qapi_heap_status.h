/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "stdint.h"
#include "qapi_status.h"
/**
 * @file qapi_heap_status.h
 *
 * @brief Heap definition
 *
 * @details This file provides heap status definition and API.
 */

#ifndef _QAPI_HEAP_STATUS_H_
#define _QAPI_HEAP_STATUS_H_

/** @addtogroup qapi_heap
 * @{ */

typedef struct heap_status_t
{
    uint32_t total_Bytes;
    uint32_t free_Bytes;
    uint32_t min_ever_free_bytes;

    uint32_t lwip_total_Bytes;
    uint32_t lwip_free_Bytes;
    uint32_t lwip_min_ever_free_bytes;

    uint32_t lwip_total_pool;
    uint32_t lwip_free_pool;
    uint32_t lwip_min_ever_free_pool;

}heap_status;

/**
 * @brief Get the memory heap status: Total bytes and Free bytes.
 *
 * @param[out] hs  pointer to hold the  heap status structure
 *
 * @return QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Heap_Status(heap_status *hs);

/** @} */

#endif /* _QAPI_HEAP_STATUS_H_ */
