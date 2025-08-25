/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "qapi_types.h"

#include "qat_util.h"
//#include "qat_pal.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Variables and Constants
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Converts a hex character to an integer.

   @param[in]  Nibble  Hex character to be converted.
   @param[out] Output  Pointer to where the converted number will be stored upon
                       a successful return.

   @return
    - true if the nibble was converted successfully.
    - false if there was an error.
*/
qbool_t QAT_Hex_Nibble_To_Int(uint8_t Nibble, uint8_t *Output)
{
    qbool_t Ret_Val;

    /* Convert the number to lower case to simplify the check for
       characters 'a' through 'f'. */
    Nibble |= 0x20;

    if ((Nibble >= '0') && (Nibble <= '9')) {
        *Output = Nibble - '0';
        Ret_Val = true;
    } else if ((Nibble >= 'a') && (Nibble <= 'f')) {
        *Output = Nibble - 'a' + 10;
        Ret_Val = true;
    } else {
        Ret_Val = false;
    }

    return (Ret_Val);
}

/**
   @brief Parses a string as an arbitrary length hex value.

   The output will be an array of the nibbles in Big Endian format. A leading
   '0x' is optional.

   @param[in]     String      NULL terminated string to be converted.
   @param[in,out] OutputSize  Size of the output buffer.  Upon successful
                              return, this value will be set to the number of
                              bytes filled.
   @param[out]    Output      Pointer to where the value will be stored upon
                              successful return.

   @return
    - true if the string was converted successfully.
    - false if there was an error.
*/
qbool_t QAT_Hex_String_To_Array(char *String, uint32_t *OutputSize, uint8_t *Output)
{
    qbool_t Ret_Val;
    uint8_t Temp_Val1;
    uint8_t Temp_Val2;
    uint32_t InputSize;

    /* Strip off the leading "0x" if present. */
    if ((String[0] == '0') && ((String[1] | 0x20) == 'x')) {
        String += 2;
    }

    memset(Output, 0, *OutputSize);
    InputSize = 0;
    Ret_Val = true;

    /* Loop until the end of the string is reached or the number is flagged as
       invalid. */
    while ((String[0] != '\0') && (Ret_Val) && (InputSize < *OutputSize)) {
        /* Make sure the next Nibble is also not NULL. */
        if (String[1] != '\0') {
            Ret_Val = QAT_Hex_Nibble_To_Int(String[0], &Temp_Val1) && QAT_Hex_Nibble_To_Int(String[1], &Temp_Val2);
            if (Ret_Val) {
                *Output = (Temp_Val1 << 4) | Temp_Val2;

                Output++;
                InputSize++;
                String += 2;
            }
        } else {
            Ret_Val = false;
        }
    }

    if (Ret_Val) {
        *OutputSize = InputSize;
    }

    return (Ret_Val);
}

/**
   @brief Verifies if a given command line parameter is a valid integer in the
          specified range.

   @param[in] Parameter  Command line parameter to verify.
   @param[in] MinValue   Minimum acceptable value for the parameter.
   @param[in] MaxValue   Maximum acceptable value for the parameter.

   @return
    - true  if the parameter is valid.
    - false if the parameter is not valid.
*/
qbool_t QAT_Verify_Integer_Parameter(int Parameter, int32_t MinValue, int32_t MaxValue)
{
    qbool_t Ret_Val;

    Ret_Val = (qbool_t)((Parameter >= MinValue) && (Parameter <= MaxValue));

    return (Ret_Val);
}

/**
   @brief Verifies if a given command line parameter is a valid unsigned integer
          in the specified range.

   This function is similar to Verify_Integer_Parameter() but is able to
   verify parameters that will be treated as unsigned 32-bit integers.

   @param[in] Parameter  Command line parameter to verify.
   @param[in] MinValue   Minimum acceptable value for the parameter.
   @param[in] MaxValue   Maximum acceptable value for the parameter.

   @return
    - true  if the parameter is valid.
    - false if the parameter is not valid.
*/
qbool_t QAT_Verify_Unsigned_Integer_Parameter(uint32_t Parameter, uint32_t MinValue, uint32_t MaxValue)
{
    qbool_t Ret_Val;

    Ret_Val = (qbool_t)((Parameter >= MinValue) && (Parameter <= MaxValue));

    return (Ret_Val);
}
