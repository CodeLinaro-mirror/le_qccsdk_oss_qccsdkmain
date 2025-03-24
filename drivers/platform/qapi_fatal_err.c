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
   @brief read the coredump info from rram.

   @param[out] coredump_buf    the buffer that stores the coredump info

   @return
    QAPI_OK -- successful reconstruction of core dump structure
    Error code -- If there is an error.
===========================================================================*/
qapi_Status_t qapi_coredump_read(coredump_type *coredump_buf)
{
  if (NULL == coredump_buf)
  {
    return QAPI_ERROR;
  }

  int32_t ret = 0;
  uint32_t coredump_addr_offset = 0;
  wifi_fw_coredump_header_t wifi_fw_coredump_header;

  /* read the header info from rram */
  ret = qapi_rram_read(COREDUMP_PARTID, 0, (uint8_t *)&wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("read coredump header info failed\n");
    return QAPI_ERROR;
  }

  coredump_addr_offset = wifi_fw_coredump_header.coredump_addr_offset;

  /* read coredump info from rram */
  ret = qapi_rram_read(COREDUMP_PARTID, coredump_addr_offset, (uint8_t *)coredump_buf, sizeof(coredump_type));

  if (ret != QAPI_OK)
  {
    printf("read coredump info failed\n");
    return QAPI_ERROR;
  }
  return QAPI_OK;
}


/*===========================================================================
   @brief write the coredump info from rram.

   @param[out] coredump_buf --  the buffer that stores the coredump info

   @return
    QAPI_OK -- successfully write the core dump info into rram
    Error code -- If there is an error.
===========================================================================*/
qapi_Status_t qapi_coredump_write(coredump_type *coredump_buf)
{
  if (NULL == coredump_buf)
  {
    return QAPI_ERROR;
  }

  int32_t ret = 0;
  uint32_t coredump_addr_offset = 0;
  wifi_fw_coredump_header_t wifi_fw_coredump_header;

  /* read the header info from rram */
  ret = qapi_rram_read(COREDUMP_PARTID, 0, (uint8_t *)&wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("read coredump header info failed\n");
    return QAPI_ERROR;
  }

  coredump_addr_offset = wifi_fw_coredump_header.coredump_addr_offset;

  /* read coredump info from rram */
  ret = qapi_rram_write(COREDUMP_PARTID, coredump_addr_offset, (uint8_t *)coredump_buf, sizeof(coredump_type));

  if (ret != QAPI_OK)
  {
    printf("write coredump info failed\n");
    return QAPI_ERROR;
  }
  return QAPI_OK;
}


/*===========================================================================
   @brief set the ramdump print flag, control the printed ram info after 
    crash

   @param[in] qapi_set_ramdump_flag   if print all the ram info
    0: specific ram info is not printed after crash
    1: specific ram info is printed after crash

   @return
    QAPI_OK -- successful reconstruction of core dump structure
    Error code -- If there is an error.
===========================================================================*/
qapi_Status_t qapi_set_ramdump_print_flag(int ramdump_print_flag)
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

/*===========================================================================
   @brief set the overwrite flag, for sequential crash, choices can be only 
    save the crash info of the first crash

   @param[in] qapi_set_ramdump_flag   if print all the ram info
    ture: overwrite the coredump info
    false: do not overwrite the coredump info

   @return
    QAPI_OK -- successful set the overwrite flag
    Error code -- If there is an error.
===========================================================================*/
qapi_Status_t qapi_set_coredump_overwrite_flag(int coredump_overwrite_flag)
{
  int ret = 0;
  wifi_fw_coredump_header_t wifi_fw_coredump_header;

  /* read the coredump header from rram */
  ret = qapi_rram_read(COREDUMP_PARTID, 0, (uint8_t *)&wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));
  if (ret != QAPI_OK)
  {
    printf("read coredump header info failed\n");
    return QAPI_ERROR;
  }

  if (coredump_overwrite_flag == 0)
  {
    wifi_fw_coredump_header.magic_num = WIFi_FW_COREDUMP_MAGIC_NUMBER_0;
    printf("set ramdump print flag 0, only record the next first coredump info\n");
  }
  else
  {
    printf("set ramdump print flag 1, coredump info will be updated when a new crash happens\n");
    wifi_fw_coredump_header.magic_num = 0;
  }
  
  /* write wifi_fw_coredump_header back to rram */
  qapi_rram_write(COREDUMP_PARTID, 0, &wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("write wifi fw coredump header failed\n");
    return QAPI_ERROR;
  }
  else
  {
    printf("set coredump overwrite flag success\n");
  }
  return QAPI_OK;
}
#endif


