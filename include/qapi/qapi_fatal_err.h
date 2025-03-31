/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __QAPI_FATAL_ERR_H__ 
#define __QAPI_FATAL_ERR_H__

/*=================================================================================
 *
 *                            FATAL ERROR MANAGER
 *
 *===============================================================================*/
 /** @file qapi_fatal_err.h
 *
 * @addtogroup qapi_fatal_err
 * @{
 *
 * @brief Fatal Error Manager (FEM)
 *
 * @details Complex software systems often run into unrecoverable error
 *          scenarios. These fatal errors cause the system to
 *          abruptly abort execution, since there is no recovery path. By
 *          nature, fatal errors are difficult to debug because detailed
 *          information related to the error is not preserved. The fatal
 *          error manager (FEM) service provides its clients a way to handle
 *          unrecoverable errors in a graceful debug-friendly fashion. It
 *          exposes a macro which, when called after a catastrophic error,
 *          preserves pertinent information to aid in debug before resetting
 *          the system.
 *
 * @code {.c}
 *
 *    * The code snippet below demonstrates the use of this interface. The example
 *    * dynamically allocates a region of memory, failing in which it 
 *    * asserts the code. This macro populates the debug information in a global
 *    * variable 'coredump' with line number, file name, and user parameters.
 *    * It also dumps the contents of general purpose registers and invokes
 *    * various user callbacks before resetting the system. The header file 
 *    * qapi_fatal_err.h should be included before calling the macro.
 *
 *   char * c;
 *
 *   c = malloc(sizeof(char));
 *   if ( c == NULL )
 *   {
 *     QAPI_FATAL_ERR(0,0,0);
 *   }
 *
 * @endcode
 *
 * @}
 */

/*==================================================================================
  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
==================================================================================*/

/*==================================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

$Header: //components/rel/core.ioe/1.0/v1/rom/release/api/debugtools/err/qapi_fatal_err.h#3 $

when       who     what, where, why
--------   ---     -----------------------------------------------------------------
10/30/15   din     Updated documentation.
09/29/15   din     Initial version.
==================================================================================*/

/*==================================================================================

                               INCLUDE FILES

==================================================================================*/

/*==================================================================================

                                   MACROS

==================================================================================*/

/* Some compilers predefine __FILENAME__; other predefine __FILE__ */
#if !defined(__FILENAME__)
#define __FILENAME__ __FILE__
#endif



/*==================================================================================

                               TYPE DEFINITIONS

==================================================================================*/

/** @addtogroup qapi_fatal_err
@{ */

/**
* Debug information structure.
*
* This structure is used to capture the module name and line number in the 
* source file where a fatal error was detected. Reference to an instance of
* this structure is passed as a parameter to the qapi_err_fatal_internal()
*          function. 
*/
typedef struct 
{
  const char  *fname;
  /**< Pointer to the source file name. */ 
  
  uint16_t     line;
  /**< Line number in the source module. */
}qapi_Err_const_t;


/**
 * @file qapi_fatal_err.h
 *
 */


/** @} */ /* end_addtogroup qapi_fatal_err */

/*==================================================================================

                            FUNCTION DECLARATIONS

==================================================================================*/

/*==================================================================================
  FUNCTION      qapi_err_fatal_internal
==================================================================================*/

/** @addtogroup qapi_fatal_err
@{ */

/**
 * Fatal error handler.
 *
 * This function implements back-end functionality supported by macro
 * QAPI_FATAL_ERR. It preserves debug information at a well-known location
 * (typically a global variable "coredump"). Preserved information captures
 * the source module name and line number, user-provided values, and contents
 *          of general purpose registers for underlying CPU architecture. After
 *          invoking several notification callbacks, it resets the system.
 *
 * @param[in] err_const Reference to the structure record line number and module name.
 * @param[in] param1    Client-provided parameter saved with debug information.
 * @param[in] param2    Client-provided parameter saved with debug information.
 * @param[in] param3    Client-provided parameter saved with debug information.
 *
 * @note This function does not return. It should only be used to gracefully
 *       handle unrecoverable errors and restart the system. Clients should 
 *       not call the function directly. Instead, they should use the macro
 *       QAPI_FATAL_ERR to access the functionality to ensure that all
 *       relevant debug information is carried forward.
 */
void qapi_err_fatal_internal
(
  const qapi_Err_const_t * err_const, 
  uint32_t                 param1, 
  uint32_t                 param2, 
  uint32_t                 param3 
);

/** @} */ /* end_addtogroup qapi_fatal_err */ 



/*==================================================================================
  MACRO         QAPI_FATAL_ERR
==================================================================================*/

/** @addtogroup qapi_fatal_err
@{ */

/**
 * Fatal error handler macro.
 *
 * This function allows for graceful handling of fatal errors. It
 * preserves information related to fatal crashes at a well-known location
 * (typically a global variable "coredump"). Preserved information captures
 * the source module name and line number, user-provided values, and contents
 * of general purpose registers used by the underlying CPU architecture.
 * After invoking several notification callbacks, it resets the system. @newpage
 *
 * @param[in] param1   User-provided parameter to be logged in coredump.
 * @param[in] param2   User-provided parameter to be logged in coredump.
 * @param[in] param3   User-provided parameter to be logged in coredump.
 *
 * @note1hang This macro does not return. It should only be used to gracefully
 *       handle unrecoverable errors and restart the system.
 @hideinitializer */
#define QAPI_FATAL_ERR(param1,param2,param3)                             \
do                                                                       \
{                                                                        \
   static const qapi_Err_const_t xx_err_const = {(const char  *)__FILENAME__, (uint16_t)__LINE__};\
   qapi_err_fatal_internal(&xx_err_const, (uint32_t)param1,(uint32_t)param2,(uint32_t)param3);         \
}while (0)

#endif

#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
#include "err.h"
#include "errlog.h"
#include "qapi_status.h"

#define qapi_m4_coredump_type coredump_type

/*==================================================================================
  FUNCTION         qapi_coredump_read
==================================================================================*/
/**
 *
* @brif Read M4 core RAM information from MISC0.
* 
* @param[out]Pointer to structure, to get m4 core dump info read out. 
@param[in] flags  Control flags for core dump info retrieval. Currently unused.

@return
status QAPI_OK on successful reconstruction of core dump structure,otherwise 
appropriate error. 
 **/
qapi_Status_t qapi_coredump_read(qapi_m4_coredump_type *m4_dump_info, int flag);



/*==================================================================================
  FUNCTION         qapi_set_ramdump_flag
==================================================================================*/
/**
 *   
 * @brief set the ramdump print flag, control the printed ram info after 
 *        crash
 *
 * @param[in] ramdump_print_flag   if print all the ram info
 *        0: specific ram info is not printed after crash
 *        1: specific ram info is printed after crash
 *
 * @return
 *       QAPI_OK -- successful set the ramdump print flag
 *       Error code -- If there is an error.
 **/
 qapi_Status_t qapi_set_ramdump_flag(int ramdump_print_flag);
#endif