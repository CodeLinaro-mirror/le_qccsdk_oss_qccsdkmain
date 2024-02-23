/*
 */

#ifndef __QCLI_API_H__  // [
#define __QCLI_API_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_console.h"

/**
   @brief Prints a formated string to the CLI.

   This function will also replace newline characters ('\n') with the string
   specified by PAL_OUTPUT_END_OF_LINE_STRING.

   @param[in] QCLI_Handle   Handle of the QCLI group that is printing the
                            string.
   @param[in] Format        Formated string to be printed.
   @param[in] ...           Variatic parameter for the format string.
*/
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *format, ...);

QAPI_Console_Group_Handle_t QCLI_Register_Command_Group(QAPI_Console_Group_Handle_t Parent_Group, const QAPI_Console_Command_Group_t *Command_Group);

#endif   // ] #ifndef __QCLI_API_H__

