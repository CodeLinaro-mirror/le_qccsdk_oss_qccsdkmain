/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef ERR_H
#define ERR_H

/*===========================================================================

                    Error Handling Common Header File

Description
  The error handling services (ERR) provide user error handling services
  through an assert-like ERR_FATAL macro.  Calling this macro indicates an
  unrecoverable error. Other services are provided as defined by the APIs
  in this header file.

===========================================================================*/

#include "com_dtypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(_PACKED_START)
    #if (defined(__CC_ARM) || defined(__GNUC__))
        /* ARM and GCC compilers */
        #define _PACKED_START
    #elif (defined(__ICCARM__))
    /* IAR compiler */
        #define _PACKED_START       __packed
    #else
        #define _PACKED_START
    #endif
#endif

#if !defined(_PACKED_END)
    #if (defined(__CC_ARM) || defined(__GNUC__))
        /* ARM and GCC compilers */
        #define _PACKED_END  __attribute__ ((packed))
    #elif (defined(__ICCARM__))
        /* IAR compiler */
        #define _PACKED_END
    #else
        #define _PACKED_END
    #endif
#endif

extern const char err_generic_msg[];

/* Type used for post-crash callbacks from error handler */
typedef void (*err_cb_ptr)(void);

#define WIFI_FW_COREDUMP_PARTID 5                                                               /* partition id                          */
#define WIFI_FW_MISC0_START_ADDR 0x36b000                                                       /* MISC0 start addr                      */
#define WIFI_FW_COREDUMP_HEADER_OFFSET 64                                                       /* coredump header offset                */
#define WIFI_FW_COREDUMP_HEADER_START_ADDRESS (WIFI_FW_MISC0_START_ADDR + WIFI_FW_COREDUMP_HEADER_OFFSET)      /* coredump header start addr             */
#define WIFI_FW_COREDUMP_ADDRESS_OFFSET 256                                                    /* coredump info addr offset compared to header */
#define WIFI_FW_COREDUMP_ADDR (WIFI_FW_MISC0_START_ADDR + WIFI_FW_COREDUMP_ADDRESS_OFFSET)        /* coredump info start addr                     */



/* -------------------------------------------------------------------------
   Function Definitions
   ------------------------------------------------------------------------- */


/*===========================================================================

FUNCTION ERR_INIT

DESCRIPTION
  This function initializes error related functionality.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
  void err_init();

  void coredump_print();



/*=========================================================================

FUNCTION err_crash_cb_register

DESCRIPTION
  Registers a function (ptr type err_cb_ptr) to be called after an ERR_FATAL
  Function should NOT rely on any messaging, task switching (or system calls
  that may invoke task switching), interrupts, etc.
 
  !!!These functions MUST NOT call ERR_FATAL under ANY circumstances!!!

DEPENDENCIES
  None
 
RETURN VALUE
  TRUE if function added to table successfully
  FALSE if function not added.
  
SIDE EFFECTS
  None

===========================================================================*/
/* Type used for post-crash callbacks from error handler */
typedef void (*err_cb_ptr)(void);

boolean err_crash_cb_dereg(err_cb_ptr cb);
boolean err_crash_cb_register(err_cb_ptr cb);


/*===========================================================================

FUNCTION ERR_FATAL_JETTISON_CORE
DESCRIPTION
  Used by ERR_FATAL to populates coredump structure. Do not call directly.
  If not using ERR_FATAL, call err_fatal_core_dump instead.

============================================================================*/
void err_fatal_jettison_core (char* err_msg, unsigned int line, const char *file_name,
   uint32 param1, uint32 param2, uint32 param3);

/*===========================================================================

MACRO ERR_FATAL

DESCRIPTION
  Log an error to error log, update the cache (for debug purpose only)
  and RESET THE SYSTEM.  Control DOES NOT RETURN to caller.  

DEPENDENCIES
  None

RETURN VALUE
  None.  Control does NOT return to caller.

SIDE EFFECTS
  None.

===========================================================================*/


/* Prototypes register core dump function */
extern void __patch__err_jettison_core_m4__jettison_core(void);

/* Prototypes stack registers core dump function for exceptions */
extern void __patch__err_jettison_core_m4__err_dump_regs_from_stack(void);

#define ERR_FATAL_EXCEPTION(Msg, line, file)                                          \
    do {                                                                       \
      __patch__err_jettison_core_m4__err_dump_regs_from_stack();                       \
      __patch__err_jettison_core_m4__jettison_core();                                  \
      coredump_fault_handler (Msg, line, file, 0, 0, 0);                         \
    } while (0);

#define ERR_FATAL_ASSERTION(Msg, line, file)                                          \
    do {                                                                   \
      __patch__err_jettison_core_m4__jettison_core();                                                     \
      coredump_fault_handler (Msg, line, file, 0, 0, 0);                         \
    } while (0);

typedef _PACKED_START struct
{
  uint16     line;
  const char *fname;    /*!< Pointer to source file name */
} _PACKED_END err_const_type;

#define _XX_MSG_CONST(xx_fmt) \
        static const err_const_type xx_msg_const = { \
              __LINE__, __FILENAME__}

void err_Fatal_internal3 (const err_const_type* const_blk, uint32 code1, uint32 code2, uint32 code3);
void err_Fatal_internal2 (const err_const_type* const_blk, uint32 code1, uint32 code2);
void err_Fatal_internal1 (const err_const_type* const_blk, uint32 code1);
void err_Fatal_internal0 (const err_const_type* const_blk);


  #define ERR_FATAL(format, code1, code2, code3)  \
   ERR_FATAL_VAR(format, 3, code1, code2, code3)


  #define ERR_SAVE_FATAL_PAR(Msg, NbPar, Par1, Par2, Par3)  \
  switch (NbPar)                                         \
  {                                                      \
   case 2:                                               \
   err_Fatal_internal2 (Msg, Par1, Par2);                     \
   break;                                                \
   case 1:                                               \
   err_Fatal_internal1 (Msg, Par1);                           \
   break;                                                \
   case 0:                                               \
   err_Fatal_internal0 (Msg);                                 \
   break;                                                \
   case 3:                                               \
   default:                                              \
   err_Fatal_internal3 (Msg, Par1, Par2, Par3);               \
   break;                                                \
  }


  #define ERR_FATAL_VAR(format, NbPar, code1, code2, code3)                     \
    do                                                                       \
    {                                                                        \
        _XX_MSG_CONST( err_generic_msg );    \
        ERR_SAVE_FATAL_PAR (&xx_msg_const, NbPar, (uint32)(code1), (uint32)(code2), (uint32)(code3))  \
     /*lint -save -e{717} */                                                 \
     } while (0)                                                             \
     /* lint -restore */


#ifdef __cplusplus
}
#endif

#endif /* ERR_H */
