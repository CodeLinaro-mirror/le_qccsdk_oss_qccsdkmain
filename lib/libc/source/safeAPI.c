/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "safeAPI.h"


size_t  memscpy(
          void        *dst,
          size_t      dst_size,
          const void  *src,
          size_t      src_size
          )

{
  size_t  copy_size = (dst_size <= src_size)? dst_size : src_size;
  memcpy(dst, src, copy_size);
  return copy_size;
}

size_t memsmove(
          void        *dst,
          size_t      dst_size,
          const void  *src,
          size_t      src_size
          )
{
   size_t  copy_size = (dst_size <= src_size)? dst_size : src_size;
   memmove(dst, src, copy_size);
   return copy_size;
}
