/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef ERRLOG_H
#define ERRLOG_H
/*===========================================================================

                   L O G  P A C K E T S  F O R  E R R

DESCRIPTION
  This header file contains the definitions of log structure for core dump
 
===========================================================================*/

/* -----------------------------------------------------------------------
** Includes 
** ----------------------------------------------------------------------- */
#include "errlog_armm.h"

#include "err.h"



/* -----------------------------------------------------------------------
** Types
** ----------------------------------------------------------------------- */

typedef enum
{
  ERR_ARCH_UNKNOWN = 0,
  ERR_ARCH_ARM,
  ERR_ARCH_QDSP6,
  SIZEOF_ERR_ARCH_TYPE
} err_arch_type;

typedef enum
{
  ERR_OS_UNKNOWN = 0,
  ERR_OS_REX,
  ERR_OS_L4, 
  ERR_OS_BLAST, 
  ERR_OS_QURT,
  ERR_OS_ARMVS, 
  SIZEOF_ERR_OS_TYPE
} err_os_type;


typedef struct
{
  err_arch_type type;
  uint32 version;
  union arch_coredump_union regs;
} arch_coredump_type;

/* update this version whenever ARM arch_coredump_type changes */
#define ERR_ARCH_COREDUMP_VER  1
#define ERR_ARCH_COREDUMP_TYPE ERR_ARCH_ARM
#define ERR_ARCH_ARM_INUSE

/************************************************************************
 *                        OS_COREDUMP_TYPES
 ************************************************************************/
#define ERR_OS_TCB_TYPE void

typedef struct
{
  err_os_type type;
  uint32 version;  
  ERR_OS_TCB_TYPE* tcb_ptr; 
} os_coredump_type;

/* update this version whenever L4 os_coredump_type changes */
#define ERR_OS_COREDUMP_VER  1
#define ERR_OS_COREDUMP_TYPE ERR_OS_QURT


/************************************************************************
 *                         ERR_COREDUMP_TYPE
 ************************************************************************/

#define ERR_LOG_MAX_MSG_LEN     80
#define ERR_LOG_MAX_FILE_LEN    50 
#define ERR_LOG_NUM_PARAMS       3 
#define ERR_IMAGE_VERSION_SIZE 128

#ifndef ERR_MAX_PREFLUSH_CB
#define ERR_MAX_PREFLUSH_CB 5
#endif /* ERR_MAX_PREFLUSH_CB */

/************************************************************************
 *                         ERR_COREDUMP_VERSION
 ************************************************************************/
#define COREDUMP_VER_MAJOR 0
#define COREDUMP_VER_MINOR 1
#define COREDUMP_VER_COUNT 1

typedef struct
{
  err_cb_ptr    err_cb;
  uint64        cb_start_tick;
} err_cb_preflush_external_type;

typedef struct
{
  char qc_image_version_string[50];
  char image_variant_string[50];
} image_coredump_type;

typedef struct
{
  uint32                        version;
  uint32                        linenum;
  uint64                        err_handler_start_time;
  uint64                        err_handler_end_time;
  char                          filename[ERR_LOG_MAX_FILE_LEN];
  char                          message[ERR_LOG_MAX_MSG_LEN];
  uint32                        param[ERR_LOG_NUM_PARAMS];
  err_cb_ptr                    err_current_cb;
  const err_const_type *        compressed_ptr;
  boolean                       err_reentrancy;
} err_coredump_type;

/* update this version whenever err_coredump_type changes */
#define ERR_COREDUMP_VER  1


/************************************************************************
 *                           COREDUMP_TYPE
 ************************************************************************/


typedef struct
{
  uint32              version;
  arch_coredump_type  arch;
  os_coredump_type    os;
  err_coredump_type   err;
  image_coredump_type image;
} coredump_type;

 /* update this version whenever coredump_type changes */
#define ERR_COREDUMP_VERSION  1

#endif /* ERRLOG_H */
