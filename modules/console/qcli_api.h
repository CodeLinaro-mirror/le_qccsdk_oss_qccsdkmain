/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier:
 * BSD-3-Clause-Clear
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

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Enumeration representing the valid error codes that can be returned by the
   command functions.
*/
typedef enum {
    QCLI_STATUS_SUCCESS_E, /**< Indicates the command executed successfully. */
    QCLI_STATUS_ERROR_E,   /**< Indicates there was an error parsing the command. */
    QCLI_STATUS_USAGE_E    /**< Indicates there was a usage error with one of the command's arguments. */
} QCLI_Command_Status_t;

/**
   Type representing a group handle.
*/
typedef void *QCLI_Group_Handle_t;

/**
   Information for a single parameter entered into the command line.
*/
typedef struct QCLI_Parameter_s {
    char *String_Value;       /**< String value entered into the command line. */
    int32_t Integer_Value;    /**< Integer value of the command line argument if the string value could be
                                   successfully converted. */
    qbool_t Integer_Is_Valid; /**< Flag indicating if the integer value is valid. */
} QCLI_Parameter_t;

/**
   @brief Type definition of a function which processes commands from the
          command line.

   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of paramters entered into the command line.

   @return
    - QCLI_STATUS_SUCCESS_E if the command executed successfully.
    - QCLI_STATUS_ERROR_E if the command encounted a general error. Note
      that the CLI currently doesn't take any action for this error.
    - QCLI_STATUS_USAGE_E indicates that the parameters passed to the CLI
      were not correct for the command.  When this error code is returned,
      the CLI will display the usage message for the command.
*/
typedef QCLI_Command_Status_t (*QCLI_Command_Function_t)(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);

/**
   Information for a single command in a command list.
*/
typedef struct QCLI_Command_s {
    QCLI_Command_Function_t
        Command_Function;       /**< Function that will be called when the command is executed from the CLI. */
    const char *Command_String; /**< String representation of the command. */
    const char *Usage_String;   /**< Usage string for the command. */
    const char *Description;    /**< Description string for the commmand. */
} QCLI_Command_t;

/**
   Information for a command group that can be registered with the command line
   interface.
*/
typedef struct QCLI_Command_Group_s {
    const char *Group_String;           /** String representation of the group. */
    uint32_t Command_Count;             /** Number of commands in the group. */
    const QCLI_Command_t *Command_List; /** List of commands for the group. */
} QCLI_Command_Group_t;
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

QAPI_Console_Group_Handle_t QCLI_Register_Command_Group(QAPI_Console_Group_Handle_t Parent_Group,
                                                        const QAPI_Console_Command_Group_t *Command_Group);

#endif  // ] #ifndef __QCLI_API_H__
