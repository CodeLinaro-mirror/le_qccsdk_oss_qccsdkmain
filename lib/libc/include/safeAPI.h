/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#include <string.h>

size_t memscpy(void  * dst, size_t dst_size, const void* src, size_t src_size);

size_t memsmove(void  * dst, size_t dst_size, const void* src, size_t src_size);

int strncpy_s(char* dst, size_t dstSizeInBytes, const char* src, size_t maxCount);
