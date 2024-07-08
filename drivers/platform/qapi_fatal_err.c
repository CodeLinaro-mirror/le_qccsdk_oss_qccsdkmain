/*
*/

/**
 * @file qapi_fatal_err.c
 *
 */
#include "qapi_types.h"

#include "qapi_fatal_err.h"

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


