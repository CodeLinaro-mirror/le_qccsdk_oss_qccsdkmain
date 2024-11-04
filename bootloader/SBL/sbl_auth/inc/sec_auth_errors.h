/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef BOOT_ERRORS_H
#define BOOT_ERRORS_H
/**===========================================================================
 **
 **                        PRIMARY BOOT LOADER
 **                        -------------------
 ** FILE
 **     boot_errors.h
 **
 ** GENERAL DESCRIPTION
 **     This header file contains declarations and type definitions related
 **     to PBL error handling.
 **
 **==========================================================================*/


/*=============================================================================

                           EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  when       who            what, where, why
  --------   ----------     ---------------------------------------------------
  03/27/15   as             Added support to make XBL shared data region as RO
  03/27/15   md             Added Errors for SPI.
  07/25/14   as             Added updates to support making XBL buffer region
                            as executable before jumping to XBL
  08/27/13   mdesai         APPS PBL Error Logging changes and removed pbl_add_log().
  05/29/13   mdesai         Adding PBL_LOG_GEN_MEMCPY and PBL_LOG_GEN_MEMZI variables
                            in pbl_log_general_type enum.
  05/27/12   tnk            Adding alignment error type during sbl loading.
  03/02/12   ak             Changing error logs structure.
  02/22/12   dxiang         Initial Revision for MSM8974

============================================================================*/


/******************************************************************************
                                MODULE INCLUDES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

#include "sec_comdef.h"

/******************************************************************************
                             PUBLIC MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
/* These variables are also used in Modem PBL .s file, due to limitation of assembler not recognizing enum's it is defined here*/
#define PBL_MEM_RW_WR_ERR           (0x8300)

/**===========================================================================
 **
 ** MACRO DESCRIPTION
 **     Given a boolean expression, this macro will verify the
 **     condition is TRUE and do nothing if the condition is TRUE,
 **     otherwise it will call the PBL error handler.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType :
 **     Param    : xx_exp
 **                This is expression that is used to evaluate
 **                if the error handler is to be called or returned.
 **
 **     Type     : [IN]
 **     DataType :
 **     Param    : xx_err_code
 **                Error code
 **
 **     Type     : [IN]
 **     DataType :
 **     Param    : xx_err_details
 **                Error details
 **
 ** RETURN VALUE
 **     None.
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/

#define PBL_VERIFY( xx_exp, xx_err_code, xx_err_details )                        \
            do {                                                    \
              if(!(xx_exp))                                      \
              {                                                    \
                pbl_error_handler( __FILE__, __LINE__,             \
                                   (xx_err_code), (xx_err_details));                 \
              }                                                    \
            } while(0)


#define PBL_VERIFY_FATAL(xx_exp, xx_err_code, xx_err_details)   \
            do{                                                    \
              if(!(xx_exp))                                      \
              {                                                    \
                pbl_fatal_error();                 \
              }                                                    \
            } while(0)


/* this is used before we check for wakeup boot, since the memory might be collapsed, so we can't clear it */
#define PBL_VERIFY_INITIAL(xx_exp)   \
            do{                                                    \
              if(!(xx_exp))                                      \
              {                                                    \
                pbl_clr_regs_and_loop();                 \
              }                                                    \
            } while(0)

/*===========================================================================
                      PUBLIC DATA DECLARATIONS
===========================================================================*/

/* This is useful to return errors from functions
 * For functionality return errors:
 * Use high 7 bits to denote standard errors (apart from sign bit)
 * Rest of 24 bits (low order) can be used for specific errors.
 * Typical human tendency is to use errors from 1, counting on up.
 */
#define PBL_NO_ERR             (0x0)

#define PBL_ERR_LOG_SIGNATURE  (0xE0000000)

#define PBL_REC_ERR_MAXBOUND        (0x7F00)  /* Recoverable Errors range from 0x1 to 0x7F */
#define PBL_NON_REC_ERR_MAXBOUND    (0xFF00)  /* Non Recoverable Errors range from 0x80 to 0xFF */

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
typedef enum
{
   PBL_INVALID_ARG       = 0x1000000,
   PBL_UNINITIALIZED     = 0x2000000,
   PBL_TIMEOUT           = 0x3000000,
   PBL_OUTSIDE_RANGE     = 0x4000000,
   PBL_DEPENDENCY_FAIL   = 0x5000000,
   PBL_SECURITY_FAIL     = 0x6000000,
   PBL_INVALID_OPTION    = 0x7000000,
   PBL_INVALID_SIZE      = 0x8000000,
   PBL_MAIN_FORCE32BITS  = 0x7FFFFFFF
}pbl_log_main_type;


typedef enum
{
  PBL_LOG_GENR           = 0x010000,
  PBL_LOG_PROC           = 0x020000,
  PBL_LOG_LOADER         = 0x030000,
  PBL_LOG_FUSE           = 0x040000,
  PBL_LOG_AUTH           = 0x050000,
  PBL_LOG_TIMER          = 0x060000,
  PBL_LOG_CLOCK          = 0x070000,
  PBL_LOG_SEC_HW         = 0x080000,
  PBL_LOG_SECBOOT        = 0x090000,
  PBL_LOG_SEC_IMG_AUTH   = 0x0A0000,
  PBL_LOG_SDCC           = 0x0B0000,
  PBL_LOG_SAHARA         = 0x0C0000,
  PBL_LOG_NAND           = 0x0D0000,
  PBL_LOG_PCIe           = 0x0E0000,
  PBL_LOG_MAIN_TYPE      = 0x0F0000,
  PBL_LOG_USB            = 0x100000,
  PBL_LOG_EXCEPTION      = 0x110000,
  PBL_LOG_ELF            = 0x120000,
  PBL_LOG_SPI            = 0x130000,
  PBL_LOG_FWD            = 0x140000,
  PBL_LOG_FORCE32BITS    = 0x7FFFFFFF /* To ensure it's 32 bits wide */
}pbl_log_block_type;


typedef enum
{
  PBL_LOG_GEN_NO_ERR                   = 0x0000,
  PBL_LOG_GEN_REC_RANGE_SIZE_ERR       = 0x0100,
  PBL_LOG_GEN_REC_FLASH_STATUS_ERR     = 0x0200,
  PBL_LOG_GEN_REC_MISC_ERR             = 0x0300,
  PBL_LOG_GEN_REC_FUNC_EXEC_ERR        = 0x0600,
  PBL_LOG_GEN_NO_IMAGES_ERR            = 0x0700,
  PBL_LOG_GEN_USB_ERR                  = 0x0800,
  PBL_LOG_GEN_QSPI_ERR                 = 0x0900,
  PBL_LOG_GEN_BOOT_OPTION_ERR          = 0x0A00,
  PBL_LOG_GEN_BOOT_TYPE_ERR            = 0x0B00,
  PBL_LOG_GEN_REC_RESERVED_127         = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_GEN_NON_REC_NULL_PTR_ERR     = 0x8000,
  PBL_LOG_GEN_NON_REC_STACK_CPY_ERR    = 0x8100,
  PBL_LOG_GEN_NON_REC_BOOT_OPTION_ERR  = 0x8200,
  PBL_LOG_GEN_NON_REC_MEM_RW_WR_ERR    = PBL_MEM_RW_WR_ERR,
  PBL_LOG_GEN_NON_REC_RESERVED_127     = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_GEN_FORCE32BITS              = 0x7FFFFFFF
} pbl_log_general_type;


typedef enum
{
  PBL_LOG_PROC_NO_ERR  	                        = 0x0000,
  PBL_LOG_PROC_REC_RANGE_SIZE_ERR               = 0x0100,
  PBL_LOG_PROC_MAIN_ERR                         = 0x0200,
  PBL_LOG_PROC_REC_RESERVED_127        	      = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_PROC_NON_REC_NULL_PTR_ERR    	      = 0x8000,
  PBL_LOG_PROC_NON_REC_RANGE_ERR       	      = 0x8100,
  PBL_LOG_PROC_NON_REC_PAGE_TABLE_ERR  	      = 0x8200,
  PBL_LOG_PROC_NON_REC_MMU_ERR         	      = 0x8300,
  PBL_LOG_PROC_NON_REC_CACHE_LOCK_ERR           = 0x8400,
  PBL_LOG_PROC_NON_REC_L3_CONFIG_TIMEOUT_ERR    = 0x8500,
  PBL_LOG_PROC_NON_REC_RESERVED_127    	      = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_PROC_FORCE32BITS             	      = 0x7FFFFFFF
} pbl_log_proc_type;


typedef enum
{
  PBL_LOG_LOADER_NO_ERR                 = 0x0000,
  PBL_LOG_LOADER_REC_RANGE_SIZE_ERR     = 0x0100,
  PBL_LOG_LOADER_REC_IMG_HDR_ERR        = 0x0200,
  PBL_LOG_LOADER_REC_LOAD_IMG_ERR       = 0x0300,
  PBL_LOG_LOADER_REC_RESERVED_127       = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_LOADER_NON_REC_NULL_PTR_ERR   = 0x8000,
  PBL_LOG_LOADER_NON_REC_RESERVED_127   = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_LOADER_FORCE32BITS            = 0x7FFFFFFF
} pbl_log_loader_type;


typedef enum
{
  PBL_LOG_AUTH_NO_ERR                    = 0x0000,
  PBL_LOG_AUTH_REC_INVALID_LENGTH_ERR    = 0x0100,
  PBL_LOG_AUTH_REC_INVALID_SIZE_ERR      = 0x0200,
  PBL_LOG_AUTH_REC_INVALID_RANGE_ERR     = 0x0300,
  PBL_LOG_AUTH_REC_INVALID_IMG_HDR_ERR   = 0x0400,
  PBL_LOG_AUTH_REC_RESERVED_127          = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_AUTH_NON_REC_NULL_POINTER_ERR  = 0x8000,
  PBL_LOG_AUTH_NON_REC_RESERVED_127      = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_AUTH_FORCE32BITS               = 0x7FFFFFFF /* To ensure it's 32 bits wide */
}pbl_log_auth_type;


typedef enum
{
  PBL_LOG_SEC_HW_NO_ERR                 = 0x0000,
  PBL_LOG_SEC_HW_REC_RESERVED_127       = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SEC_HW_NON_REC_MISC_ERR       = 0x8200,
  PBL_LOG_SEC_HW_NON_REC_RESERVED_127   = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SEC_HW_FORCE32BITS            = 0x7FFFFFFF /* To ensure it's 32 bits wide */
} pbl_log_sec_hw_type;


typedef enum
{
  PBL_LOG_SECBOOT_NO_ERR                     = 0x0000,
  PBL_LOG_SECBOOT_REC_MISC_ERR               = 0x0100,
  PBL_LOG_SECBOOT_REC_RESERVED_127           = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SECBOOT_NON_REC_NULL_PTR_ERR       = 0x8000,
  PBL_LOG_SECBOOT_NON_REC_RESERVED_127       = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SECBOOT_FORCE32BITS                = 0x7FFFFFFF
} pbl_log_secboot_type;


typedef enum
{
  PBL_LOG_SEC_IMG_AUTH_NO_ERR                = 0x0000,
  PBL_LOG_SEC_IMG_REC_DRIVER_ERR             = 0x0100,
  PBL_LOG_SEC_IMG_REC_MISC_ERR               = 0x0200,
  PBL_LOG_SEC_IMG_REC_ELF_HEADER_INVALID_ERR = 0x0300,
  PBL_LOG_SEC_IMG_REC_RESERVED_127           = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SEC_IMG_NON_REC_NULL_PTR_ERR       = 0x8000,
  PBL_LOG_SEC_IMG_NON_REC_RESERVED_127       = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SEC_IMG_FORCE32BITS                = 0x7FFFFFFF
} pbl_log_sec_img_auth_type;


typedef enum
{
  PBL_LOG_TIMER_NO_ERR                   = 0x0000,
  PBL_LOG_TIMER_REC_RESERVED_127         = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_TIMER_NON_REC_NULL_PTR_ERR     = 0x8000,
  PBL_LOG_TIMER_NON_REC_MISC_FAILURE_ERR = 0x8100,
  PBL_LOG_TIMER_NON_REC_RESERVED_127     = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_TIMER_FORCE32BITS              = 0x7FFFFFFF
}pbl_log_timer_type;


typedef enum
{
  PBL_LOG_CLOCK_NO_ERR                     = 0x0000,
  PBL_LOG_CLOCK_REC_RESERVED_127       	   = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_CLOCK_NON_REC_NULL_PTR_ERR   	   = 0x8000,
  PBL_LOG_CLOCK_NON_REC_DRIVER_FAILURE_ERR = 0x8100,
  PBL_LOG_CLOCK_NON_REC_RESERVED_127       = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_CLOCK_FORCE32BITS                = 0x7FFFFFFF
}pbl_log_clock_type;


typedef enum
{
  PBL_LOG_SDC_NO_ERR                    = 0x0000,
  PBL_LOG_SDC_REC_SPEC_VER_ERR          = 0x0100,
  PBL_LOG_SDC_REC_BLK_LEN_ERR           = 0x0200,
  PBL_LOG_SDC_REC_SETUP_XFER_ERR        = 0x0300,
  PBL_LOG_SDC_REC_READ_DATA_ERR         = 0x0400,
  PBL_LOG_SDC_REC_ALIGNMENT_ERR         = 0x0500,
  PBL_LOG_SDC_REC_OUT_OF_BOUND_ERR      = 0x0600,
  PBL_LOG_SDC_REC_RANGE_ERR             = 0x0700,
  PBL_LOG_SDC_REC_GPT_VALIDATE_ERR      = 0x0800,
  PBL_LOG_SDC_REC_GPT_HDR_CRC_ERR       = 0x0900,
  PBL_LOG_SDC_REC_GPT_ENTRY_SIZE_ERR    = 0x0A00,
  PBL_LOG_SDC_REC_RESERVED_127          = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SDC_NON_REC_NULL_PTR_ERR      = 0x8000,
  PBL_LOG_SDC_NON_REC_SPEED_SELECT_ERR  = 0x8100,
  PBL_LOG_SDC_NON_REC_INIT_CLK_ERR      = 0x8200,
  PBL_LOG_SDC_NON_REC_BUS_WIDTH_ERR     = 0x8300,
  PBL_LOG_SDC_NON_REC_INVALID_PORT_ERR  = 0x8400,
  PBL_LOG_SDC_NON_REC_INVALID_OFFSET_ERR= 0x8500,
  PBL_LOG_SDC_NON_REC_RESERVED_127      = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SDC_FORCE32BITS               = 0x7FFFFFFF
} pbl_log_sdc_type;


typedef enum
{
  PBL_LOG_SAHARA_NO_ERR                 = 0x0000,
  PBL_LOG_SAHARA_REC_ALIGNMENT_ERR      = 0x0100,
  PBL_LOG_SAHARA_REC_RANGE_ERR          = 0x0200,
  PBL_LOG_SAHARA_REC_IMG_ID_ERR         = 0x0300,
  PBL_LOG_SAHARA_REC_IMG_HDR_ERR        = 0x0400,
  PBL_LOG_SAHARA_REC_RESERVED_127       = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SAHARA_NON_REC_NULL_PTR_ERR   = 0x8000,
  PBL_LOG_SAHARA_NON_REC_PROTOCOL_ERR   = 0x8100,
  PBL_LOG_SAHARA_NON_REC_IMAGE_TYPE_ERR = 0x8200,
  PBL_LOG_SAHARA_NON_REC_ENUM_ERR       = 0x8300,
  PBL_LOG_SAHARA_NON_REC_CTRL_ERR		= 0x8400,
  PBL_LOG_SAHARA_NON_REC_RESERVED_127   = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SAHARA_FORCE32BITS            = 0x7FFFFFFF
} pbl_log_sahara_type;



typedef enum
{
  PBL_FUSE_RPM_PBL_BOOT_SPEED 	     = 0x0100,
  PBL_FUSE_APPS_PBL_BOOT_SPEED       = 0x0200,
  PBL_FUSE_FAST_BOOT                 = 0x0300,
  PBL_FUSE_SDCC_MCLK_BOOT_FREQ       = 0x0400,
  PBL_FUSE_AP_SW_REV                 = 0x0500,
  PBL_FUSE_USB_ENUM_TIMEOUT          = 0x0600,
  PBL_LOG_FUSE_REC_RESERVED_127      = PBL_REC_ERR_MAXBOUND,
  PBL_FUSE_NON_REC_QSPI_BOOT_FREQ    = 0x8000,
  PBL_LOG_FUSE_NON_REC_RESERVED_127  = PBL_NON_REC_ERR_MAXBOUND,
  PBL_FUSE_LOG_FORCE32BITS           = 0x7FFFFFFF
}pbl_log_fuse_type;


typedef enum
{
  PBL_LOG_ELF_NO_ERR                        = 0x0000,
  PBL_LOG_ELF_REC_RANGE_SIZE_ERR            = 0x0100,
  PBL_LOG_ELF_REC_INVALID_SEGMENT_ERR       = 0x0200,
  PBL_LOG_ELF_REC_ALIGNMENT_ERR             = 0x0300,
  PBL_LOG_ELF_REC_HASH_SEGMENT_MISMATCH_ERR = 0x0400,
  PBL_LOG_ELF_REC_HASH_AUTH_FAILURE_ERR     = 0x0500,
  PBL_LOG_ELF_REC_READ_DEVICE_FAILURE_ERR   = 0x0600,
  PBL_LOG_ELF_REC_ELF_CLASS_FORMAT_ERR      = 0x0700,
  PBL_LOG_ELF_REC_RESERVED_127              = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_ELF_NON_REC_NULL_PTR_ERR          = 0x8000,
  PBL_LOG_ELF_NON_REC_INIT_FAIL_ERR		    = 0x8100,
  PBL_LOG_ELF_NON_REC_RESERVED_127          = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_ELF_FORCE32BITS                   = 0x7FFFFFFF
} pbl_log_elf_type;


typedef enum
{
  PBL_LOG_SPI_NO_ERR                    = 0x0000,
  PBL_LOG_SPI_REC_READ_DATA_ERR         = 0x0100,
  PBL_LOG_SPI_REC_OUT_OF_BOUND_ERR      = 0x0200,
  PBL_LOG_SPI_REC_RANGE_ERR             = 0x0300,
  PBL_LOG_SPI_REC_GPT_VALIDATE_ERR      = 0x0400,
  PBL_LOG_SPI_REC_GPT_HDR_CRC_ERR       = 0x0500,
  PBL_LOG_SPI_REC_GPT_ENTRY_SIZE_ERR    = 0x0600,
  PBL_LOG_SPI_REC_WRITE_DATA_ERR        = 0x0700,
  PBL_LOG_SPI_REC_ENTER_4BYTE_MODE_ERR  = 0x0800,
  PBL_LOG_SPI_REC_ENABLE_QUAD_MODE_ERR  = 0x0900,
  PBL_LOG_SPI_REC_ENTER_HPO_MODE_ERR    = 0x0A00,
  PBL_LOG_SPI_REC_RESERVED_127          = PBL_REC_ERR_MAXBOUND,
  PBL_LOG_SPI_NON_REC_NULL_PTR_ERR      = 0x8000,
  PBL_LOG_SPI_NON_REC_BUS_WIDTH_ERR     = 0x8100,
  PBL_LOG_SPI_NON_REC_INVALID_PORT_ERR  = 0x8200,
  PBL_LOG_SPI_NON_REC_INVALID_STATE_ERR = 0x8300,
  PBL_LOG_SPI_NON_REC_INIT_CLK_ERR      = 0x8400,
  PBL_LOG_SPI_NON_REC_INVALID_OFFSET_ERR= 0x8500,
  PBL_LOG_SPI_NON_REC_DUMMY_CYCLE_ERR   = 0x8600,
  PBL_LOG_SPI_NON_REC_DMA_ERR           = 0x8700,
  PBL_LOG_SPI_NON_REC_DMA_TIMEOUT_ERR   = 0x8800,
  PBL_LOG_SPI_NON_REC_XIP_TIMEOUT_ERR   = 0x8900,
  PBL_LOG_SPI_NON_REC_RESERVED_127      = PBL_NON_REC_ERR_MAXBOUND,
  PBL_LOG_SPI_FORCE32BITS               = 0x7FFFFFFF
} pbl_log_spi_type;


typedef enum
{
  PBL_LOG_FWD_NO_ERR                    = 0x0000,
  PBL_LOG_FWD_NON_INFO_ERR              = 0x0100,
  PBL_LOG_FWD_PREV_ERR                  = 0x0200,
  PBL_LOG_FWD_FORCE32BITS               = 0x7FFFFFFF
} pbl_log_fwd_type;


typedef enum _pbl_mem_region_id
{
   PBL_MEM_REGION_VALID_ID                  = 0,
   PBL_MEM_REGION_INVALID_ID                = 0x7FFFFFFF
} pbl_mem_region_id;


/* Error log structure to store data describing error  */
typedef struct boot_pbl_err_type
{
   uint32                  pbl_err_code_start;  /* Error code used as limiter to define
                                                   block for which error is being logged
                                                   and to ensure that log was completely
                                                   and succesfully written. */
  uint32                   pbl_err_details;     /* Used to provide any additional
                                                   information about the error.*/
  uint32                   timestamp;           /* Timestamp */
  uint32                   pbl_version_number;
  const char*              filename;            /* File this function was called from. */
  uint32                   line_num;            /* Line number this function was called from. */
  uint32                   pbl_err_code_end;    /* Error code used as limiter to define
                                                   block for which error is being logged
                                                   and to ensure that log was completely
                                                   and succesfully written. */
} boot_pbl_err_type;




//--------------------------------------------------
// This is used to keep track of all iterations in PBL main loop

typedef enum
{
  PBL_HISTORY_LOG_BOOT_TYPE_PON_COLD_BOOT   = 0x0000,
  PBL_HISTORY_LOG_BOOT_TYPE_SW_COLD_BOOT    = 0x4000,
  PBL_HISTORY_LOG_BOOT_TYPE_WAKEUP_BOOT		= 0x8000,
  PBL_HISTORY_LOG_BOOT_TYPE_WATCHDOG_BOOT	= 0xC000,
  PBL_HISTORY_LOG_BOOT_TYPE_FORCE32BITS     = 0x7FFFFFFF
} pbl_history_log_boot_type;


typedef enum
{
  PBL_HISTORY_LOG_AON_VALID					= 0x0000,
  PBL_HISTORY_LOG_AON_INVALID				= 0x2000,
  PBL_HISTORY_LOG_AON_FORCE32BITS           = 0x7FFFFFFF
} pbl_history_log_aon;


typedef enum
{
  PBL_HISTORY_LOG_HANDLER_VALID					= 0x0000,
  PBL_HISTORY_LOG_HANDLER_INVALID				= 0x1000,
  PBL_HISTORY_LOG_HANDLER_FORCE32BITS			= 0x7FFFFFFF
} pbl_history_log_handler;


typedef enum
{
  PBL_HISTORY_LOG_FWD_REVERT_CURRENT			= 0x04000,
  PBL_HISTORY_LOG_FWD_REVERT_GOLDEN				= 0x08000,
  PBL_HISTORY_LOG_FWD_REVERT_FORCE32BITS		= 0x7FFFFFFF
} pbl_history_log_fwd_revert;


typedef enum
{
  PBL_HISTORY_LOG_WATCHDOG_COUNTER_VALID		= 0x00000,
  PBL_HISTORY_LOG_WATCHDOG_COUNTER_INVALID		= 0x02000,
  PBL_HISTORY_LOG_WATCHDOG_COUNTER_FORCE32BITS	= 0x7FFFFFFF
} pbl_history_log_watchdog_handler;


typedef struct boot_history_log_type
{
	uint32				data;
} boot_history_log_type;

/******************************************************************************
                         PUBLIC FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function is the error handler for the PBL. It handles errors
 **     occurring during cold boot.
 **     1. Reinit instruction and data caches, as well as MMU.
 **        Redo L2 line locking.
 **
 **     2. Place debug information in RPM_CODE_RAM.
 **
 **     3. Zeroes out memory used for the RW section, ZI section, and stack.
 **
 **     4. Try to recover via loading image from SD and then USB,
 **        id feature is enabled
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : char *
 **     Param    : filename_ptr
 **                This is File pointer this function was called from -
 **                defined by __FILE__
 **
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : line
 **                This is the Line number this function was called
 **                from - defined by  __LINE__
 **
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : err_code
 **                Error code to indicate which error.
 **
 **     Type     : [IN]
 **     DataType : uint64
 **     Param    : err_details
 **                Error details providing additional information
 **                about error.
 **
 ** RETURN VALUE
 **     None.
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
extern void pbl_error_handler
(
  const char*   filename_ptr,
  const uint32  line,
  const uint32  err_code,
  uint32  err_details
);


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function is PBL fatal error handler
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **                None
 **
 ** RETURN VALUE
 **     None
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
void pbl_fatal_error(void);

/*===========================================================================

FUNCTION  PBL_CHECK_VALID_ADDRESS_RANGE

DESCRIPTION
  Check if provided address range lies in accepted address range.

DEPENDENCIES
  None

RETURN VALUE
  TRUE : ADDR VALID, lies within accepted address range
  FALSE: ADDR INVALID, intersects with address outside accepted range

SIDE EFFECTS
  None

===========================================================================*/
extern boolean pbl_check_valid_address_range(uint32 start_addr, uint32 size);

#endif  /* BOOT_ERRORS_H */

