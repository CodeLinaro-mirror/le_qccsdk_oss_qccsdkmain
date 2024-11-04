/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __QAPI_CONSOLE_H__ // [
#define __QAPI_CONSOLE_H__

/**
 * @file qapi_console.h
 *
 * @brief QAPI Console interface definition
 *
 * @details This file provides the base type definitions used by the QAPI console.
 */


/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/** @name Firmware Upgrade Codes
 *
 * The following definitions represent the status codes of the Firmware Upgrade module.
 * @{
 */

#define QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR         ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_CONSOLE, 1))) /**< Console command generic error. */
#define QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE         ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_CONSOLE, 2))) /**< Return usage for console command. */
#define QAPI_ERROR_CONSOLE_INIT_FAIL                    ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_CONSOLE, 3))) /**< Fail to init console. */
#define QAPI_ERROR_CONSOLE_REGISTER_CMD_GROUP_FAIL      ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_CONSOLE, 4))) /**< Fail to register command group. */
#define QAPI_ERROR_CONSOLE_REGISTER_CMD_FAIL            ((qapi_Status_t)(__QAPI_ERROR(QAPI_MOD_CONSOLE, 5))) /**< Failed to register command. */
/** @} */ /* end namegroup */

typedef void *QAPI_Console_Handle_t;

typedef void *QAPI_Console_Group_Handle_t;

typedef struct QAPI_Console_Parameter_s
{
   char    *String_Value;
   int32_t  Integer_Value;
   qbool_t  Integer_Is_Valid;
} QAPI_Console_Parameter_t;

typedef qapi_Status_t (*QAPI_Concole_Command_Function_t)(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

typedef struct QAPI_Console_Command_s
{
   QAPI_Concole_Command_Function_t  Command_Function; /** The function that will be called when the command is executed from the CLI. */
   const char                       *Command_String;   /** The string representation of the function.                                  */
   const char                       *Usage_String;     /** The usage string for the command.                                           */
   const char                       *Description;      /** The description string for the commmand.                                    */
} QAPI_Console_Command_t;

typedef struct QAPI_Console_Command_Group_s
{
   const char                   *Group_String;   /** The string representation of the group. */
   uint32_t                     Command_Count; /** The number of commands in the group.    */
   const QAPI_Console_Command_t *Command_List;   /** The list of commands for the group.     */
} QAPI_Console_Command_Group_t;

/**
   @brief This function is used to register a command group with the CLI.

   @param Parent_Group is the group which this group should be registerd
          under as a subgroup.  If this parameter is NULL, then the group
          will be registered at the top level.
   @param Command_Group is the command group to be registered.  Note that
          this function assumes the command group information will be
          constant and simply stores a pointer to the data.  If the command
          group and its associated information is not constant, its memory
          MUST be retained until the command is unregistered.

   @return
    - THe handle for the group that was added.
    - NULL if there was an error registering the group.
*/
QAPI_Console_Group_Handle_t QAPI_Console_Register_Command_Group(QAPI_Console_Group_Handle_t Parent_Group, const QAPI_Console_Command_Group_t *Command_Group);

/**
   @brief This function is used to register a command with the CLI.

   @param Parent_Group is the group which this group should be registerd
          under as a subgroup.  If this parameter is NULL, then the command
          will be registered at the top level.
   @param Command is the command to be registered.

   @return
    - THe handle for the command that was added.
    - NULL if there was an error registering the command.
*/
QAPI_Console_Handle_t QAPI_Console_Register_Command(QAPI_Console_Group_Handle_t Parent_Group, const QAPI_Console_Command_t *Command);

/* Function Definitions to be added later */

#endif // ] #ifndef __QAPI_CONSOLE_H__
