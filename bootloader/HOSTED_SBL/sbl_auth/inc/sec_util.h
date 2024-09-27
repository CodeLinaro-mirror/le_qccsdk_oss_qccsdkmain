/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef BOOT_UTIL_H_
#define BOOT_UTIL_H_
/**===========================================================================
 **
 **                        PRIMARY BOOT LOADER
 **                        -------------------
 ** FILE
 **     pbl_util.h
 **
 ** GENERAL DESCRIPTION
 **     This file contains utility functions use don both M4F and M0.
 **
 **
 **
 **==========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

  This section contains comments describing changes made to this file.
  Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/16/11   dxiang  Clean up
===========================================================================*/

#undef  WORDSIZE
#define WORDSIZE   sizeof(int)
#undef  WORDMASK
#define WORDMASK   (WORDSIZE - 1)

/******************************************************************************
                         PUBLIC FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function takes a byte pointer and it returns the byte value in
 **     in 32 bits.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : const uint8 *
 **     Param    : p
 **                Byte Pointer
 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                Byte value in 32 bits.
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 bLoad8 (const uint8 *p);


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **    This function takes 32 bits input value and stores the 8LSB at the address
 **    pointed by the input byte pointer.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : uint8 *
 **     Param    : p
 **                Byte Pointer
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : val
 **                32 bit data from which the bit[7:0] will be stored in memory.
 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                Byte value in 32 bits.
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
void bStor8 (uint8 *p, uint32 val);


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **    This function compares two buffers byte by byte.
 **
 **    This function replaces the memcmp utility in the C library and
 **    ensures that read opeartions from memory are 32 bit aligned.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : const uint8*
 **     Param    : value1_ptr
 **                Pointer to buffer 1
 **     Type     : [IN]
 **     DataType : const uint8*
 **     Param    : value2_ptr
 **                Pointer to buffer 2
 **     DataType : const uint32
 **     Param    : byte_len
 **                Lenght to compare
 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                0 if two buffers are the same
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 bByteCompare
(
  const uint8*  value1_ptr,
  const uint8*  value2_ptr,
  const uint32  byte_len
);


/*===========================================================================
FUNCTION qmemcpy

DESCRIPTION
  This function copies a block of memory, handling overlap.
  This routine implements memcpy, and memmove. And ensures
  that read and write operations from/to memory are 32 bit
  aligned.

PARAMETERS
  source_ptr           - Pointer to source buffer.
  destination_ptr      - Pointer to destination buffer.
  len                  - Length to copy.

RETURN VALUE
  None.

===========================================================================*/
void qmemcpy(void* destination_ptr, const void* source_ptr, uint32 len);


/*===========================================================================
FUNCTION qmemset

DESCRIPTION
  This function set a block of memory to a given value.

  This routine implements memset and ensures that read
  and write operations from/to memory are 32 bit aligned
  This function sets Len bytes of destination to input value.


PARAMETERS
  dst_ptr         - Pointer to destination buffer.
  val             - Value  to be set
  len             - Lenght to set

RETURN VALUE
  None.

===========================================================================*/
void qmemset (
  void   *dst_ptr,
  uint32 val,
  uint32 len
);


/*===========================================================================
FUNCTION num_of_bits_set

DESCRIPTION
  This function counts the number of "1's" in the 32-bit data passed.

PARAMETERS
  data_to_check

RETURN VALUE
  Count of number of "1's" in the data word

===========================================================================*/
uint32 num_of_bits_set (uint32 data_to_check);


#endif /* __BOOT_UTIL_H */

