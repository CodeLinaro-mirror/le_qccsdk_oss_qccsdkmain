/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COM_UTILS_H
#define COM_UTILS_H

/**
  @file com_utils.h
  @brief This header file contains general utils that are of use to all modules.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _COMPILE_ASSERT
#define _COMPILE_ASSERT(predicate) extern uint8_t _dummy_array[(predicate) ? 1 : -1];
#endif /* WIFI_FW_COMPILE_ASSERT */

#ifndef _STRUCT_4BYTE_ALLIGN_CHECK
#define _STRUCT_4BYTE_ALLIGN_CHECK(struct) _COMPILE_ASSERT(!(sizeof(struct) % 4))
#endif /* _STRUCT_4BYTE_ALLIGN_CHECK */

#ifndef _STRUCT_SIZE_SYNC
#define _STRUCT_SIZE_SYNC(struct, val) _COMPILE_ASSERT(((sizeof(struct)) == val))
#endif /* WIFI_FW_STRUCT_SIZE_SYNC */

#ifdef __cplusplus
}
#endif

/** @} */ /* end_addtogroup utils_services */
#endif    /* COM_DTYPES_H */
