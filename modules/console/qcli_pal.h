/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __QCLI_PAL_H__
#define __QCLI_PAL_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qapi_status.h"
#include <string.h>
#include <stddef.h>

#ifdef CONFIG_MATTER_ENABLE
#define configUART_COMMAND_CONSOLE_STACK_SIZE (2048)
#else
#define configUART_COMMAND_CONSOLE_STACK_SIZE (1024)
#endif
#define configUART_COMMAND_CONSOLE_TASK_PRIORITY (7U)

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/**
   Character that is inpretted as an end of line for inputs from the console.
*/
#define PAL_INPUT_END_OF_LINE_CHARACTER ('\r')

/**
   String that is used as the end of line for outputs to the console.
*/
#define PAL_OUTPUT_END_OF_LINE_STRING ("\r\n")

/**
   @brief Writes a buffer to the console.

   All data from this buffer will be written to the console or buffered locally
   before this function returns.

   @param Length[in]  Length of the data to be wr itten.
   @param Buffer[in]  Buffer to be written to the console.
*/
void PAL_Console_Write(uint32_t Length, const char *Buffer);

/**
   @brief Allocates a block of memory from the heap.

   @param[in] Size  Minimum size of the memory block to allocate.

   @return A pointer to the allocated memory or NULL if there was an error.
*/
void *PAL_Malloc(size_t Length);

/**
   @brief Frees a block of memory from the heap.

   @param[in] Pointer  Block to free as returned by a call to PAL_Malloc().
*/
void PAL_Free(void *Pointer);

/**
   @brief Takes a lock for re-entrancy protection for the QCLI module.

   This module expects the lock to behave like a mutex. It should support
   recursively taking the lock and, for threaded platforms, block until the lock
   can be taken as applicable.

   @return
    - true if the lock was taken successfully
    - false if the lock was not taken successfully
*/
qbool_t PAL_Take_Lock(void);

/**
   @brief Releases a lock taken with PAL_Take_Lock().
*/
void PAL_Release_Lock(void);

#endif
