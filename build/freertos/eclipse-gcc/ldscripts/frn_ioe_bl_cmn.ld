/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
 * Memory Spaces Definitions.
 */


MEMORY
{
  OTPMEM (rw) : ORIGIN = 0x001A0000, LENGTH = 0x00001000
  PBLMEM (xrw) : ORIGIN = 0x00200000, LENGTH = 0x00008000
  FDTMEM (rw) : ORIGIN = 0x00208000, LENGTH = 0x00000400
  PBL_PATCH (xrw) : ORIGIN = 0x00208400, LENGTH = 0x00002000
  /* 
   * PBL_PATCH is divided into 2 regions, namely PBL_PATCH_RO and PBL_PATCH_RW 
   * The origin and length of the regions are consistent with MPU configuration
   * Don't modify the value of the 2 regions, otherwise the MPU should be reconfigured
   */
  PBL_PATCH_RO (xrw) : ORIGIN = 0x00208400, LENGTH = 0x00001c00
  PBL_PATCH_RW (xrw) : ORIGIN = 0x0020a000, LENGTH = 0x00000400
  SBLAMEM (xrw) : ORIGIN = 0x0020A400, LENGTH = 0x00008000
  SBLBMEM (xrw) : ORIGIN = 0x00212400, LENGTH = 0x00008000
  RRAMMEM (rw) : ORIGIN = 0x0021A400, LENGTH = 0x00165C00
  UDPARTMEM (r) : ORIGIN = 0x0036A000, LENGTH = 0x00001000
  PBL_RAM (xrw) : ORIGIN = 0x00000000, LENGTH = 0x0000A000
  SBL_RAM (xrw) : ORIGIN = 0x00080000, LENGTH = 0x00010000
  /* 
   * SBL_RAM is divided into 2 regions, namely SBL_RAM_RO and SBL_RAM_RW 
   * The origin and length of the regions are consistent with MPU configuration
   * Don't modify the value of the 2 regions, otherwise the MPU should be reconfigured
   */
  SBL_RAM_RO (xrw) : ORIGIN = 0x00080000, LENGTH = 0x00008000
  SBL_RAM_RW (xrw) : ORIGIN = 0x00088000, LENGTH = 0x00008000
  CONFIG_TABLE (rw) : ORIGIN = 0x00090000, LENGTH = 0x00010000
  FLASH_XIP (xrw) : ORIGIN = 0x003000000, LENGTH = 0x00ffffff
}

/* FDT start address*/
_FDT_Start_Addr = ORIGIN(FDTMEM);

/* SBLA image start address*/
_SBLA_IMAGE_Start_Addr = ORIGIN(SBLAMEM);
