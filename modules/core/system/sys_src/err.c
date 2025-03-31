/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

         E R R O R     R E P O R T I N G    S E R V I C E S

GENERAL DESCRIPTION
  This module provides error reporting services for both fatal and
  non-fatal errors.  This module is not a task, but rather a set of
  callable procedures which run in the context of the calling task.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "erri.h"
#include "errlog.h"

#include "timer.h"
#include "nt_sys_monitoring.h"
#include "qapi_rram.h"
#include "nt_socpm_sleep.h"
#include "wifi_fw_version.h"
#include "ExceptionHandlers.h"
#include "ferm_hkadc_drv.h"
#include "nt_hw.h"
//#include "nt_hw_reg.h"
/*===========================================================================

                      Prototypes for External functions

===========================================================================*/


extern void err_update_image_versioning_info (void);
extern void jettison_core(void);
extern void sys_m_init(void);
//extern void sys_m_err_notify_peripherals(void);
extern void sys_m_wait_peripherals(void);
//extern void sys_m_end_action(void);

/*===========================================================================

                      Prototypes for internal functions

===========================================================================*/
static void err_emergency_error_recovery(void);
static void err_loop_pause(void);
static void err_flush_cache(void);
void err_invoke_action(void);



/* Used to replace all ERR_FATAL message strings.
 * Reduces code size by eliminating other const strings used for errors.
 */
 const char err_generic_msg[] = "Error Fatal, parameters: %d %d %d";
 
 /* a brif introduction to coredump related rram info*/
 const char coredump_rram_addr_info[] = "length: coredump size; value: coredump start addr";

 /* Struct used to hold coredump data */
 coredump_type * coredump = NULL;

 int g_ramdump_print_flag = 0;
 wifi_fw_coredump_header_t g_wifi_fw_coredump_header;
 
 /* Ptr used by assembly routines to grab registers */
 /*  (update this as needed if struct changes)      */
 //arch_coredump_type* arch_coredump_ptr = (arch_coredump_type*)(&coredump.arch.regs.array);
 arch_coredump_type* arch_coredump_ptr = NULL;
 
 static boolean err_services_init_complete = FALSE;
 
 err_cb_preflush_external_type err_preflush_external[ERR_MAX_PREFLUSH_CB+1];

 /* Image Versioning data for coredump */
 char QC_IMAGE_VERSION_STRING_AUTO_UPDATED[] = "";
 char IMAGE_VARIANT_STRING_AUTO_UPDATED[] = WIFI_FW_VARIANT_NAME;

/* The function tables below are processed by the error handler
 * in the following order:
 *   1. err_preflush_internal (one time)
 *   2. err_preflush_external (one time)
 *   3. err_flush_internal (one time)
 */

static const err_cb_ptr err_preflush_internal[] =
{
  //sys_m_err_notify_peripherals,
  /* NULL must be last in the array */
  NULL
};


static const err_cb_ptr err_flush_internal[] =
{
  err_flush_cache,
  err_invoke_action,
  err_loop_pause,
  /* NULL must be last in the array */
  NULL
};



/*===========================================================================

                              Function definitions

===========================================================================*/


/*===========================================================================

FUNCTION ERR_INIT

DESCRIPTION
  Initializes error specific data .

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None

===========================================================================*/

void err_init (void * coredump_ptr)
{
  //sys_m_init();

  if ( coredump_ptr == NULL )
  {
    coredump_ptr = (void *)malloc(sizeof(coredump_type));
  }

  if (coredump_ptr == NULL)
    printf("coredump malloc failed\n");

  int ret = 0;
  coredump = coredump_ptr;
  arch_coredump_ptr = (arch_coredump_type*)(&(coredump->arch.regs.array));

  /* Cannot update anything in the coredump structure before the actual error 
   * handling starts*/ 

  memset( &(err_preflush_external), 0,
        (sizeof(err_cb_preflush_external_type) * ERR_MAX_PREFLUSH_CB+1));

  /* magic num should not be changed unless be modified by qapi */
  ret = qapi_rram_read(COREDUMP_PARTID, 0, (uint8_t *)&g_wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("read wifi fw coredump header failed\n");
  }
  
  //g_wifi_fw_coredump_header.magic_num = 0;

  (void) strlcpy((char *)&(g_wifi_fw_coredump_header.tlv_desc), coredump_rram_addr_info, 64);
  g_wifi_fw_coredump_header.tlv.length = sizeof(coredump_type);
  g_wifi_fw_coredump_header.tlv.value = COREDUMP_ADDR;

  g_wifi_fw_coredump_header.coredump_part_id = COREDUMP_PARTID;
  g_wifi_fw_coredump_header.coredump_addr_offset = COREDUMP_ADDRESS_OFFSET;
  g_wifi_fw_coredump_header.coredump_size = sizeof(coredump_type);
  g_wifi_fw_coredump_header.coredump_start_addr = COREDUMP_ADDR;
  
  /* coredump header is saved at the beginning of RAMDUMPMEM */
  qapi_rram_write(COREDUMP_PARTID, 0, &g_wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

  if (ret != QAPI_OK)
  {
    printf("write wifi fw coredump header failed\n");
  }

  err_services_init_complete = TRUE;

} /* err_init */

/*===========================================================================

FUNCTION ERR_DEINIT

DESCRIPTION
  De-Initializes error specific callback data .

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None

===========================================================================*/

void err_deinit (void)
{
  /*  ERR_MAX_PREFLUSH_CB+1 is used to be on par with coredump structure */
  memset( &(err_preflush_external), 0,
        (sizeof(err_cb_preflush_external_type) * ERR_MAX_PREFLUSH_CB+1));

  err_services_init_complete = FALSE;
		  
} /* err_deinit */

void coredump_info_print(coredump_type *coredump)
{
  if (coredump == NULL)
  {
    printf("coredump info is NULL\n");
  }
  else
  {
    printf( "\r\n");
    printf( "============== coredump start ==============\r\n");
    printf( "coredump version = [%d]\r\n", coredump->version);
    printf( "os type=[%d]\r\n", coredump->os.type);
    printf( "os version = [%d]\r\n", coredump->os.version);
    printf( "arch version = [%X]\r\n", coredump->arch.version);
    printf( "error message = [%s]\r\n", (char*)coredump->err.message);
    printf( "file name = [%s]\r\n", (char*)coredump->err.filename);
    printf( "line number = [%d]\r\n",coredump->err.linenum);
    printf( "param[0] = [%d]\r\n",coredump->err.param[0]);
    printf( "param[1] = [%d]\r\n",coredump->err.param[1]);
    printf( "param[2] = [%d]\r\n",coredump->err.param[2]);

    printf( "reg info:\r\n"
      "R0    %08X   R1    %08X\r\n"
      "R2    %08X   R3    %08X\r\n"
      "R12   %08X   LR    %08X\r\n"
      "PC    %08X   PSR   %08X\r\n",
      coredump->arch.regs.name.r0, coredump->arch.regs.name.r1,
      coredump->arch.regs.name.r2, coredump->arch.regs.name.r3,
      coredump->arch.regs.name.r12, coredump->arch.regs.name.lr,
      coredump->arch.regs.name.pc, coredump->arch.regs.name.psr);
    printf( "ICSR  %08X   VTOR  %08X\r\n"
      "AIRCR %08X   SCR   %08X \r\n"
      "CCR   %08X\r\n" ,
      coredump->arch.regs.name.icsr,coredump->arch.regs.name.vtor,
      coredump->arch.regs.name.aircr, coredump->arch.regs.name.scr,
      coredump->arch.regs.name.ccr);
    printf( "SHPR1 %08x  SHPR2 %08x \r\n"
      "SHPR3 %08x\r\n",
      coredump->arch.regs.name.shpr1, coredump->arch.regs.name.shpr2,
      coredump->arch.regs.name.shpr3);
    printf( "SHCSR %08X   CFSR  %08X \r\n"
      "HFSR  %08X   DFSR  %08X \r\n"
      "MMFAR %08X\r\n",
      coredump->arch.regs.name.shcsr, coredump->arch.regs.name.cfsr,
      coredump->arch.regs.name.hfsr, coredump->arch.regs.name.dfsr,
      coredump->arch.regs.name.mmfar);
    printf( "BFAR  %08X   AFSR %08X \r\n"
      "PFR0  %08X   PFR1 %08X \r\n"
      "DFR   %08X\r\n",
      coredump->arch.regs.name.bfar, coredump->arch.regs.name.afsr,
      coredump->arch.regs.name.pfr0, coredump->arch.regs.name.pfr1,
      coredump->arch.regs.name.dfr);
    printf( "ADR     %08X   MMFR[0] %08X \r\n"
      "MMFR[1] %08X   MMFR[2] %08X \r\n"
      "MMFR[3] %08X\r\n",
      coredump->arch.regs.name.adr, coredump->arch.regs.name.mmfr[0],
      coredump->arch.regs.name.mmfr[1], coredump->arch.regs.name.mmfr[2],
      coredump->arch.regs.name.mmfr[3]);
    printf( "ISAR[0] %08X  ISAR[1] %08X \r\n"
      "ISAR[2] %08X  ISAR[3] %08X \r\n"
      "ISAR[4] %08X  CPACR   %08X\r\n",
      coredump->arch.regs.name.isar[0], coredump->arch.regs.name.isar[1],
      coredump->arch.regs.name.isar[2], coredump->arch.regs.name.isar[3],
      coredump->arch.regs.name.isar[4], coredump->arch.regs.name.cpacr);
    printf( "NVIC_ISPR0 %08X   NVIC_ISPR1 %08X   NVIC_ISPR2 %08X\r\n",
      coredump->arch.regs.name.ispr[0], coredump->arch.regs.name.ispr[1],
      coredump->arch.regs.name.ispr[2]);
    printf( "NVIC_ISER0 %08X   NVIC_ISER1 %08X   NVIC_ISER2 %08X\r\n",
      coredump->arch.regs.name.iser[0], coredump->arch.regs.name.iser[1],
      coredump->arch.regs.name.iser[2]);
    printf( "============== coredump end ==============\r\n");
    printf( "\r\n");
  }
  
}

/*===========================================================================

FUNCTION ERR_UPDATE_IMAGE_VERSIONING_INFO

DESCRIPTION
  Updates Image versioning info to coresump

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/

void err_update_image_versioning_info (void)
{
  static char image_info_string[50] = {0};
  snprintf((char *)image_info_string, 50, "build date and time: %s - %s", __DATE__, __TIME__);
  memcpy(coredump->image.qc_image_version_string, image_info_string, 50);
  memset(image_info_string, 0, 50);

  snprintf((char *)image_info_string, 50, "coredump version: %d.%d.%d\t%s", COREDUMP_VER_MAJOR, COREDUMP_VER_MINOR, COREDUMP_VER_COUNT, WIFI_FW_VARIANT_NAME);
  memcpy(coredump->image.image_variant_string, image_info_string, 50);
  memset(image_info_string, 0, 50);
} /* err_update_image_versioning_info */

/*===========================================================================

FUNCTION ERR_GET_TIMETICK

DESCRIPTION
  Add timetick value to specified address if timetick handle is initialized

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None .

===========================================================================*/

static void err_get_timetick(uint64 * tick)
{
   *tick = hres_timer_timetick_get();

} /* err_get_timetick */

/*===========================================================================

FUNCTION ERROR_FATAL_HANDLER

DESCRIPTION
  This function is invoked from err_fatal_jettison_core. When using JTAG,
  default breakpoint for ERR_FATAL should be placed at this function.
  Will log error to SMEM, kill the PA, and copy the coredump data into
  the err_data structure in unintialized memory.


DEPENDENCIES

RETURN VALUE
  No return.

SIDE EFFECTS
  **************************************************************
  ************ THERE IS NO RETURN FROM THIS FUNCTION ***********
  **************************************************************

===========================================================================*/
void err_fatal_handler (void)
{
    wifi_fw_coredump_header_t wifi_fw_coredump_header;
    int ret = 0;

    /* record the end time of coredump before writing RRAM */
    err_get_timetick(&(coredump->err.err_handler_end_time));

    /* print the coredump info */
    coredump_info_print(coredump);

    /* check if overwrite the coredump info */
    ret = nt_rram_read(COREDUMP_PARTITION_START_ADDRESS, &wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));

    if (ret == 0 && (wifi_fw_coredump_header.magic_num != WIFi_FW_COREDUMP_MAGIC_NUMBER_1))
    {
      /* write the coredump into the rram */
      nt_rram_write(COREDUMP_ADDR, coredump, sizeof(coredump_type));

      /* if it is the first time of crash */
      if (wifi_fw_coredump_header.magic_num == WIFi_FW_COREDUMP_MAGIC_NUMBER_0)
      {
        wifi_fw_coredump_header.magic_num = WIFi_FW_COREDUMP_MAGIC_NUMBER_1;

        /* for next time, the coredump info will not be saved */
        nt_rram_write(COREDUMP_PARTITION_START_ADDRESS,  &wifi_fw_coredump_header, sizeof(wifi_fw_coredump_header_t));
      }
    }
    /* system reboot */
    nt_system_sw_reset();
} /* err_fatal_handler */


/*===========================================================================

FUNCTION ERR_FATAL_JETTISON_CORE
DESCRIPTION
  Logs fatal error information, including a core dump.
  Not to be called directly by outside code -- for that, use the function
  err_fatal_core_dump().

  NOTE: There is no return from this function.
============================================================================*/
void err_fatal_jettison_core (
  char *err_msg,
  unsigned int line,       /* From __LINE__ */
  const char   *file_name, /* From __FILE__ */
  uint32 param1,
  uint32 param2,
  uint32 param3
)
{
  /* NOTE: register information should already be saved prior to
   * calling this function.
   */

  if (coredump == NULL)
  {
    printf("coredump is NULL\n");
    err_emergency_error_recovery();
    /* Keep KW happy, err_emergency_error_recovery is a non returning function */
    return;
  }

  memset( &(coredump->err), 0 , sizeof(err_coredump_type));
  memset( &(coredump->os), 0 , sizeof(os_coredump_type));
  memset( &(coredump->image), 0 , sizeof(image_coredump_type));
  
  err_get_timetick( &(coredump->err.err_handler_start_time));

  /* Kick Dog */
  nt_watchdog_bark_timer_reset();

  /* Set type and version values */
  coredump->version = ERR_COREDUMP_VERSION;
  coredump->arch.type = ERR_ARCH_COREDUMP_TYPE;
  coredump->arch.version = ERR_ARCH_COREDUMP_VER;
  coredump->os.type = ERR_OS_COREDUMP_TYPE;
  coredump->os.version = ERR_OS_COREDUMP_VER;
  coredump->err.version = ERR_COREDUMP_VER;

  err_update_image_versioning_info();
  
  /* Store line number */
  coredump->err.linenum = line;

  /* Copy file name */
  if(file_name != 0)
  {
    (void) strlcpy((char *)coredump->err.filename,
                       (char *)file_name,
                       ERR_LOG_MAX_FILE_LEN);
  }
  /* Copy message string */
  if (err_msg != NULL)
  {
    (void) strlcpy((char *)coredump->err.message,
                       (char *)err_msg,
                       ERR_LOG_MAX_MSG_LEN);
  }
  else if(err_generic_msg != 0)
  {
    (void) strlcpy((char *)coredump->err.message,
                       (char *)err_generic_msg,
                       ERR_LOG_MAX_MSG_LEN);
  }
  coredump->err.param[0]=param1;
  coredump->err.param[1]=param2;
  coredump->err.param[2]=param3;

  /* Call ERR_FATAL handler (no return) */
  err_fatal_handler();

} /* err_fatal_jettison_core */

/*===========================================================================

FUNCTION WIFI_FW_SAVE_COREDUMP_CORE_REGS
DESCRIPTION
  copy the reg info into related coredump area.

  NOTE: There is no return from this function.
============================================================================*/
void wifi_fw_save_coredump_core_regs(ExceptionStackFrame* frame)
{
  if (NULL != frame)
  {
    arch_coredump_field_type core_regs = {
      .r0 = frame->r0,
      .r1 = frame->r1,
      .r2 = frame->r2,
      .r3 = frame->r3,
      .r12 = frame->r12,
      .lr = frame->lr,
      .pc = frame->pc,
      .psr = frame->psr,
      .icsr = SCB->ICSR,
      .vtor = SCB->VTOR,
      .aircr = SCB->AIRCR,
      .scr = SCB->SCR,
      .ccr = SCB->CCR,
      .shpr1 = SCB->SHP[3] | (SCB->SHP[2] << 8) | (SCB->SHP[1] << 16) | (SCB->SHP[0] << 24),
      .shpr2 = SCB->SHP[7] | (SCB->SHP[6] << 8) | (SCB->SHP[5] << 16) | (SCB->SHP[4] << 24),
      .shpr3 = SCB->SHP[11] | (SCB->SHP[10] << 8) | (SCB->SHP[9] << 16) | (SCB->SHP[8] << 24),
      .shcsr = SCB->SHCSR,
      .cfsr = SCB->CFSR,
      .hfsr = SCB->HFSR,
      .dfsr = SCB->DFSR,
      .mmfar = SCB->MMFAR,
      .bfar = SCB->BFAR,
      .afsr = SCB->AFSR,
      .pfr0 = SCB->PFR[0],
      .pfr1 = SCB->PFR[1],
      .dfr = SCB->DFR,
      .adr = SCB->ADR,
      .mmfr[0] = SCB->MMFR[0],
      .mmfr[1] = SCB->MMFR[1],
      .mmfr[2] = SCB->MMFR[2],
      .mmfr[3] = SCB->MMFR[3],
      .isar[0] = SCB->ISAR[0],
      .isar[1] = SCB->ISAR[1],
      .isar[2] = SCB->ISAR[2],
      .isar[3] = SCB->ISAR[3],
      .isar[4] = SCB->ISAR[4],
      .cpacr = SCB->CPACR,
      .ispr[0] = NVIC->ISPR[0],
      .ispr[1] = NVIC->ISPR[1],
      .ispr[2] = NVIC->ISPR[2],
      .iser[0] = NVIC->ISER[0],
      .iser[1] = NVIC->ISER[1],
      .iser[2] = NVIC->ISER[2],
    };
  memcpy(&coredump->arch.regs.name, &core_regs, sizeof(arch_coredump_field_type));
  }
}

unsigned int bswap_32(unsigned int x)
{
    return ((x & 0x000000ff) << 24) |
           ((x & 0x0000ff00) << 8) |
           ((x & 0x00ff0000) >> 8) |
           ((x & 0xff000000) >> 24);
}

/*=========================================================================

FUNCTION coredump_fault_handler

DESCRIPTION
  Action to be taken when crash is happening, coredump info will be edited
  and saved at rram.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  No return from this function

===========================================================================*/

void coredump_fault_handler(
char *err_msg,
unsigned int line,       /* From __LINE__ */
const char   *file_name, /* From __FILE__ */
uint32 param1,
uint32 param2,
uint32 param3,
ExceptionStackFrame* frame
)
{
  /* check g_ramdump_print_flag and print all the ram info */
  static char raminfo_string[20] = {0};
  uint32 ram_addr = 0;
  uint32 ram_data = 0;
  if (g_ramdump_print_flag || CONFIG_WIFI_FW_RAMDUMP_PRINT_FLAG)
  {
    printf( "\r\n");
    printf( "============== ramdump start ==============\r\n");
    while(ram_addr < (uint32)0xA0000)
    {
      memset(raminfo_string, 0, strlen(raminfo_string));
      ram_data = *(uint32 *)ram_addr;
      snprintf(raminfo_string, 20, "%08x", bswap_32(ram_data));
      UART_Send_direct(raminfo_string, strlen(raminfo_string));
      ram_addr += 4;
    }
    printf( "\r\n");
    printf( "============== ramdump end ==============\r\n");
    printf( "\r\n");
  }

  /* this flag is used to prevent the case where an assertion happens after the exception */
  static boolean err_fatal_reentrancy_flag;

  /* Disable the irqs */
  //__asm volatile("cpsid i" : : : "memory");
  nt_disable_irq();  // Disable all the interrupts

  if (err_fatal_reentrancy_flag == FALSE)
  {
    err_fatal_reentrancy_flag = TRUE;

    /* Kick Dog */
    nt_watchdog_bark_timer_reset();

    /* save regs */
    wifi_fw_save_coredump_core_regs(frame);

    /* save core coredump info */
    err_fatal_jettison_core(err_msg, line, file_name, 0, 0, 0);
  }
  else
  {
    nt_system_sw_reset();
  }
}

/*=========================================================================

FUNCTION err_emergency_error_recovery

DESCRIPTION
  Action to be taken when more than one error has occurred, or if an
  error occurs before err_init has completed.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  No return from this function

===========================================================================*/

void err_emergency_error_recovery( void )
{
  /* Define action to be taken when multiple crashes occur */

  err_invoke_action();
  err_loop_pause();

} /* err_emergency_error_recovery */


/*=========================================================================

FUNCTION err_invoke_action

DESCRIPTION
  Invokes completion of the err_fatal handler.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  May not return if action is taken (dload, reset, etc.)
===========================================================================*/
void err_invoke_action( void )
{
  if ( coredump != NULL )
    err_get_timetick( &(coredump->err.err_handler_end_time));
  
  err_flush_cache();

  //sys_m_end_action();

} /* err_invoke_action */

/*=========================================================================

FUNCTION err_pause_usec

DESCRIPTION
  Pause specified duration of usec. Breaks up pause into smaller chunks
  to prevent warning msg from clk code.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void err_pause_usec(uint32 usec)
{
  uint32 i,kick_ctr=0;
  uint64_t pTimeRet;

  for(i=0; i<usec; i+=ERR_CLK_PAUSE_SMALL)
  {
    /* Break up call into smaller delays to prevent clk_pause
       warning msg. */
    timer_cvt_from_tick64(ERR_CLK_PAUSE_SMALL, T_USEC, &pTimeRet);  
    hres_timer_us_delay((uint32_t)pTimeRet);

    /* Make sure dog gets kicked on long delays */
    if(kick_ctr++ >= ERR_CLK_PAUSE_KICK)
    {
      nt_watchdog_bark_timer_reset();  /* ints are disabled, use force_kick */
      kick_ctr = 0;
    }
}
} /* err_pause_usec */


/*=========================================================================

FUNCTION err_loop_pause

DESCRIPTION
  Pause ERR_LOOP_DELAY_USEC; used at end of handling loop.
  Also kicks dog as needed.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
volatile int err_always_true =1;

static void err_loop_pause( void )
{
  static uint32 loop_count=0;

  err_flush_cache();
  
  /* Loop forever until the system is reset */
  while(err_always_true)
  {
    /* Delay for specificied time and kick the Dog*/

    /* Pause ERR_LOOP_DELAY_USEC microseconds */
    /* This time interval accounts for one 'loop' of the error handler,
     ignoring overhead from other calls */
    err_pause_usec(ERR_LOOP_DELAY_USEC);
    loop_count++;

    if(loop_count == ERR_LOOP_DOG_FREQ)
    {
      nt_watchdog_bark_timer_reset();  /* Ints are disabled, use force_kick */
      loop_count=0;
    }

  }
} /* err_loop_pause */

/*=========================================================================

FUNCTION err_crash_cb_register

DESCRIPTION
  Registers a function (ptr type err_cb_ptr) to be called after an ERR_FATAL
  Function should NOT rely on any messaging, task switching (or system calls
  that may invoke task switching), interrupts, etc.

  !!!These functions MUST NOT call ERR_FATAL/ASSERT under ANY circumstances!!!

DEPENDENCIES
  None

RETURN VALUE
  TRUE if function added to table successfully
  FALSE if function not added.

SIDE EFFECTS
  None

===========================================================================*/
boolean err_crash_cb_register(err_cb_ptr cb)
{
  int i;
  boolean rval = FALSE;

  for(i=0; i<ERR_MAX_PREFLUSH_CB; i++)
  {
	if(err_preflush_external[i].err_cb == NULL)
  {
	  err_preflush_external[i].err_cb = cb;
    rval = TRUE;
    break;
  }
  }

  return rval;
} /* err_crash_cb_register */


/*=========================================================================

FUNCTION err_crash_cb_dereg

DESCRIPTION
 Deregisters a function from the error callback table.

DEPENDENCIES
  None

RETURN VALUE
  TRUE if removed
  FALSE if function is not found in table

SIDE EFFECTS
  None

===========================================================================*/
boolean err_crash_cb_dereg(err_cb_ptr cb)
{
  int i;
  boolean rval = FALSE;

  for(i=0; i<ERR_MAX_PREFLUSH_CB; i++)
  {
	if(err_preflush_external[i].err_cb == cb)
  {
	  err_preflush_external[i].err_cb = NULL;
    rval = TRUE;
    break;
  }
  }

  return rval;
} /* err_crash_cb_dereg */

/*=========================================================================

FUNCTION err_flush_cache

DESCRIPTION
  Function which will call all necessary cache-flushing APIs.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void err_flush_cache(void)
{
  //placeholder

} /* err_flush_cache */