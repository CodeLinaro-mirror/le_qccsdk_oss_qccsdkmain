/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __QAT_H__  // [
#define __QAT_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qat_api.h"
#include "qurt_mutex.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/**
   Determines the size of the input buffer for CLI commands. It effectively
   controls the maximum length of a command and its parameters.
*/
#define QAT_MAXIMUM_COMMAND_STRING_LENGTH (2048)

/**
   Determines the maximum number of parameters that can be provided. Note that
   this may also include parameters used to navigate into groups.
*/
#define QAT_MAXIMUM_NUMBER_OF_PARAMETERS (32)

/**
   Determines the size of the buffer used for formatted messages to the console
   when using QCLI_Printf.
*/
#define QAT_MAXIMUM_PRINTF_LENGTH (2048)

/**
   Index for the first command in a command list.
*/
//#define QCLI_COMMAND_START_INDEX                   (1)
/**
   Character that is inpretted as an end of line for inputs from the console.
*/
#define PAL_INPUT_END_OF_LINE_CHARACTER ('\r')

/**
   String that is used as the end of line for outputs to the console.
*/
#define PAL_OUTPUT_END_OF_LINE_STRING ("\r\n")

/**
   Indicates received characters should be echoed to the console.
*/
#define PAL_ECHO_CHARACTERS (true)

#define AT_COMMAND_LENGTH 20

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 *-----------------------------------------------------------------------*/

/**
   @brief Initializes the QCLI module.

   This function must be called before any other QCLI functions.

   @param[in] Command_Group  Group of commands for the platform.  This can be
                             NULL if no platform commands are required.
                             Note that this function assumes the command group
                             information will be constant and simply stores a
                             pointer to the data.  If the group and its
                             associated information is not constant, its memory
                             must be retained.

   @return QCLI group handle that can be used for the platform abstraction to
           print to the console.
*/
qbool_t QAT_TxTask_Initialize(void);

/**
   @brief Passes characters input from the command line to the QCLI module for
          processing.

   @param[in] Length  Number of bytes in the provided buffer.
   @param[in] Buffer  Buffer containing the inputted data.

   @return
    - true if QCLI was initialized successfully.
    - false if initialization failed.
*/
qbool_t QAT_Process_Input_Data(uint32_t Length, char *Buffer);

qapi_Status_t QAT_Output(uint32_t Length, const char *Buffer);

/**
   Information for a command group list entry.
*/
typedef struct Group_List_Entry_s {
    struct Group_List_Entry_s *Next;   /**< Next entry in the list. */
    QAT_Command_Group_t Command_Group; /**< Command group information. */
} Group_List_Entry_t;

/**
   Context information for the QCLI module.
*/
typedef struct HTC_Context_s {
    Group_List_Entry_t *Next;         /**< Root of the group menu structure. */
    QAT_Command_t *Executing_Command; /**< Group of currently executing command. */

    uint32_t Input_Length; /**< Length of the current console input string. */
    char
        Input_String[QAT_MAXIMUM_COMMAND_STRING_LENGTH + 1]; /**< Buffer containing the current console input string. */
    QAT_Parameter_t Parameter_List[QAT_MAXIMUM_NUMBER_OF_PARAMETERS + 1]; /**< List of parameters for input command. */
    uint32_t Default_Paramter_Flag; /**< Length of the current console input string. */
    uint8_t *Command_Name;
    uint32_t Command_Flag;

    char Printf_Buffer[QAT_MAXIMUM_PRINTF_LENGTH]; /**< Buffer used for formatted output strings. */
    QAT_Tx_Queue_t *Tx_Queue;
    qurt_mutex_t mutex;
} HTC_Context_t;

/**
   Information for the result of a Find_Command() operation.
*/
typedef struct Cur_Data_Mode_Cmd_s {
    char cur_data_mode_commnd[AT_COMMAND_LENGTH];
    uint32_t Op_Type;
    uint32_t Parameter_Count;

} Cur_Data_Mode_Cmd_t;

#endif  // ] #ifndef __QAT_H__
