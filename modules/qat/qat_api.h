/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __QAT_API_H__  // [
#define __QAT_API_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qcli_api.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
/*--------------------------------------------------------------------------
                Flag of the AT Command Parser
--------------------------------------------------------------------------*/
#define QAT_STR_NA 0x00000001 /*  Name found     */
#define QAT_STR_EQ 0x00000002 /*  <=> found      */
#define QAT_STR_QU 0x00000004 /*  <?> found      */
#define QAT_STR_AR 0x00000008 /*  Argument found */

#define QAT_OP_QUERY              0x00000001
#define QAT_OP_HELP               0x00000002
#define QAT_OP_EXEC               0x00000004
#define QAT_OP_EXEC_W_PARAM       0x00000008
#define QAT_OP_EXEC_IN_DATA_MODEL 0x00000010

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/
typedef enum {
    /*-------------------------------------------------------------------------
      Numbered per ITU-T V.25 ter
    -------------------------------------------------------------------------*/
    QAT_RC_OK = 0,          /*  AT: OK            */
    QAT_RC_CONNECT = 1,     /*  AT: CONNECT       */
    QAT_RC_RING = 2,        /*  AT: RING          */
    QAT_RC_NO_CARRIER = 3,  /*  AT: NO CARRIER    */
    QAT_RC_ERROR = 4,       /*  AT: ERROR         */
    QAT_RC_NO_DIALTONE = 6, /*  AT: NO DIAL TONE  */
    QAT_RC_BUSY = 7,        /*  AT: BUSY          */
    QAT_RC_NO_ANSWER = 8,   /*  AT: NO ANSWER     */

    QAT_RC_CONNECT_W_PARAMETER = 9, /*	AT: CONNECT <parameter>	  */
    QAT_RC_QUIET = 10,              /*  AT: No Messages           */
    QAT_RC_QUIET_NO_CR = 11,        /*  AT: No Messages           */

    QAT_RC_MAX, /*	AT: Max Num   */

} QAT_Result_Enum_Type;

/**
   Enumeration representing the valid error codes that can be returned by the
   command functions.
*/
typedef enum {
    QAT_STATUS_SUCCESS_E, /**< Indicates the command executed successfully. */
    QAT_STATUS_ERROR_E,   /**< Indicates there was an error parsing the command. */
    QAT_STATUS_USAGE_E    /**< Indicates there was a usage error with one of the command's arguments. */
} QAT_Command_Status_t;

/**
   Information for a single parameter entered into the command line.
*/
typedef struct QAT_Parameter_s {
    char *String_Value;       /**< String value entered into the command line. */
    int32_t Integer_Value;    /**< Integer value of the command line argument if the string value could be
                                   successfully converted. */
    qbool_t Integer_Is_Valid; /**< Flag indicating if the integer value is valid. */
} QAT_Parameter_t;

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
typedef QAT_Command_Status_t (*QAT_Command_Function_t)(uint32_t Op_Type, uint32_t Parameter_Count,
                                                       QAT_Parameter_t *Parameter_List);
typedef QAT_Command_Status_t (*QAT_Response_Function_t)(QAT_Command_Status_t status);
/**
   Enumeration representing the valid error codes that can be returned by the
   command functions.
*/
typedef enum {
    QAT_Transfer_Mode_AT_COMMAND_E,     /**< Indicates process the AT command strings. */
    QAT_Transfer_Mode_ONLINE_DATA_E,    /**< Indicates online data state. */
    QAT_Transfer_Mode_ONLINE_COMMAND_E, /**< Indicates online command state. */
} QAT_Transfer_Mode_t;
typedef qbool_t (*QAT_Transfer_Mode_Handle_t)(uint32_t Length, uint8_t *Buffer);

typedef void (*QAT_Reply_Comp_CB_t)(void *cb_context, void *UserParam);

/**
   Information for a command group list entry.
*/
typedef struct QAT_Tx_Queue_s {
    struct QAT_Tx_Queue_s *Next; /**< Next entry in the list. */
    uint16_t Len;                /**< uart buffer length */
    uint8_t *Buffer;             /**< uart buffer. */
    QAT_Reply_Comp_CB_t CB;
    void *CB_Context;
} QAT_Tx_Queue_t;

/**
   Information for a single command in a command list.
*/
typedef struct QAT_Command_s {
    const char *Command_String; /**< String representation of the command. */
    QAT_Command_Function_t
        Command_Function;     /**< Function that will be called when the command is executed from the CLI. */
    uint32_t Command_Flags;   /**< processing type of the command. */
    const char *Command_Info; /**< String representation of the command. */
} QAT_Command_t;

/**
   Information for a command group that can be registered with the command line
   interface.
*/
typedef struct QAT_Command_Group_s {
    // const char           *Group_String;  /** String representation of the group. */
    uint32_t Command_Count;      /** Number of commands in the group. */
    QAT_Command_t *Command_List; /** List of commands for the group. */
} QAT_Command_Group_t;

extern QAT_Transfer_Mode_t QAT_Transfer_Mode;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 *-----------------------------------------------------------------------*/

/**
   @brief Registers a command group with the command line interface.

   NOTE: This function assumes the command group information will be constant
   and simply stores a pointer to the data.  If the group and its associated
   information is not constant, its memory must be retained until the group is
   unregistered.

   @param[in] Parent_Group   Group which this group should be registered under
                             as a subgroup.  If NULL, the group will be
                             registered at the top level.
   @param[in] Command_Group  Command group to be registered.

   @return
    - The handle for the group that was added.
    - NULL if there was an error registering the group.
*/
qbool_t QAT_Register_Command_Group(QAT_Command_t *Command_Group, uint32_t Number);

/**
   @brief Unregisters a command group from the command line interface.

   If the specified group has subgroups, they will be unregistered as well.

   @param[in] Group_Handle  Handle of the group to unregister.
*/
qbool_t QAT_Unregister_Command_Group(QAT_Command_t *Command_Group);

qbool_t QAT_Transfer_Mode_set(QAT_Transfer_Mode_t Mode, QAT_Transfer_Mode_Handle_t Handle);
qbool_t QAT_Data_Transfer_Mode_Handle(uint32_t Length, uint8_t *Buffer);

QAT_Command_Status_t QAT_Response_Str(int Ret_Code, char *Buffers);
QAT_Command_Status_t QAT_Response_Buffer(int Ret_Code, char *Buffer, uint32_t Length);

/**
   @brief Prints a formated string to the CLI.

   This function will also replace newline characters ('\n') with the string
   specified by PAL_OUTPUT_END_OF_LINE_STRING.

   @param[in] QCLI_Handle   Handle of the QCLI group that is printing the
                            string.
   @param[in] Format        Formated string to be printed.
   @param[in] ...           Variatic parameter for the format string.
*/
void QCLI_Printf(QCLI_Group_Handle_t Group_Handle, const char *format, ...);

void QAT_TXQ_Wakeup(uint32_t Signal);

int snprintf(char *str, size_t size, const char *format, ...);

#endif  // ] #ifndef __QCLI_API_H__
