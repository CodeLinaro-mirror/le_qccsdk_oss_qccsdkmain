/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/**
 * @file qapi_fatal_err.c
 *
 */
#include "qapi_types.h"
#include "qapi_fatal_err.h"
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
#include "ferm_qspi.h"
#include "qapi_status.h"
extern int g_ramdump_print_flag;
#endif

/*===========================================================================

FUNCTION qapi_err_fatal_internal

===========================================================================*/
void qapi_err_fatal_internal
(
   const __attribute__((__unused__))qapi_Err_const_t *  err_const, 
  uint32_t __attribute__((__unused__)) param1,
  uint32_t __attribute__((__unused__)) param2,
  uint32_t __attribute__((__unused__))param3
)
{

  *(uint32_t*)0x21000000 = 0;
  param2 = param3;

} /* qapi_err_Fatal_internal*/

#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
/*===========================================================================
  @brif Read M4 core RAM information from MISC0.

  @param[out]Pointer to structure, to get m4 core dump info read out. 
  @param[in] flags  Control flags for core dump info retrieval. Currently unused.

  @return
  status QAPI_OK on successful reconstruction of core dump structure,otherwise 
  appropriate error. 
===========================================================================*/
qapi_Status_t qapi_coredump_read(qapi_m4_coredump_type *m4_dump_info, int flags)
{
  if (NULL == m4_dump_info)
  {
    return QAPI_ERROR;
  }

  int32_t ret = 0;
  uint32_t coredump_addr_offset = 0;
  uint32_t coredump_header_offset = 0;
  misc0_header_t misc0_header;
  wifi_fw_coredump_header_t wifi_fw_coredump_header;

  /* reader the misc0 header info from ram */
  ret = qapi_rram_read(WIFI_FW_COREDUMP_PARTID, 0, (uint8_t *)&misc0_header, sizeof(misc0_header_t));

  if (ret != QAPI_OK)
  {
    printf("read misc0 header info failed\n");
    return QAPI_ERROR;
  }

  /* check the validation of magic number */
  if (misc0_header.magic_num != MISC0_MAGIC_NUM)
  {
    printf("failed, invalid misc0 header\n");
    return QAPI_ERROR;
  }

  coredump_header_offset = misc0_header.next_start_offset;

  /* read the header info from rram */
  ret = qapi_rram_read(WIFI_FW_COREDUMP_PARTID, coredump_header_offset, (uint8_t *)&wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("read coredump header info failed\n");
    return QAPI_ERROR;
  }

  coredump_addr_offset = wifi_fw_coredump_header.coredump_addr_offset;

  /* read coredump info from rram */
  ret = qapi_rram_read(WIFI_FW_COREDUMP_PARTID, coredump_addr_offset, (uint8_t *)m4_dump_info, sizeof(qapi_m4_coredump_type));

  if (ret != QAPI_OK)
  {
    printf("read coredump info failed\n");
    return QAPI_ERROR;
  }
  return QAPI_OK;
}



/*===========================================================================
   @brief set the ramdump print flag, control the printed ram info after 
    crash

   @param[in] ramdump_print_flag   if print all the ram info
    0: specific ram info is not printed after crash
    1: specific ram info is printed after crash

   @return
    QAPI_OK -- successful reconstruction of core dump structure
    Error code -- If there is an error.
===========================================================================*/
qapi_Status_t qapi_set_ramdump_flag(int ramdump_print_flag)
{
  /* ramdump print flag is controled by both ramdump_print_flag 
   * and CONFIG_WIFI_FW_RAMDUMP_PRINT_FLAG */
  if (CONFIG_WIFI_FW_RAMDUMP_PRINT_FLAG)
  {
    printf("please set CONFIG_WIFI_FW_RAMDUMP_PRINT_FLAG=0 first!\n");
    return QAPI_ERROR;
  }

  if (ramdump_print_flag == 0)
  {
    g_ramdump_print_flag = 0;
    printf("set ramdump print flag 0, specific ram info should not be printed\n");
  }
  else
  {
    g_ramdump_print_flag = 1;
    printf("set ramdump print flag 1, specific ram info should be printed\n");
  }
  
  return QAPI_OK;
}
#endif


