/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdlib.h>
#include "string.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qcli.h"
#include "qcli_api.h"
#include "qcli_pal.h"
#include "qcli_util.h"

#include "self_test.h"
#include "mbedtls/aes.h"
#include "mbedtls/ccm.h"
#include "mbedtls/dhm.h"
/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define QCC_PRINTF_HANDLE qcli_qcc_group

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
QAPI_Console_Group_Handle_t qcli_qcc_group; /* Handle for our QCLI Command Group. */

/**********************************************************************************************************/
/* Function Declarations											                                      */
/**********************************************************************************************************/
static qapi_Status_t Command_Unit_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t Qcc_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {Command_Unit_Test, "selftest", "\n selftest <Module> <verbose>",
     "Run qcc selftest, module: aes, ccm, sha1, sha256, all"},
};

const QAPI_Console_Command_Group_t Qcc_Command_Group = {
    "QCC", /* Firmware Upgrade */
    sizeof(Qcc_Command_List) / sizeof(QAPI_Console_Command_t),
    Qcc_Command_List,
};

/**********************************************************************************************************/
/* Function Definitions    											                                      */
/**********************************************************************************************************/
/* This function is used to register the Firmware Upgrade Command Group with QCLI   */
void Initialize_Qcc_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    QCC_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &Qcc_Command_Group);
    if (QCC_PRINTF_HANDLE) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "Qcc Selftest Registered \n");
    }
}

/*=================================================================================================*/

/**
   @brief This function processes the "unittest" command from the CLI.
*/

static qapi_Status_t Command_Unit_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 2) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "usage: selftest <Module> <verbose>\n");
        return QAPI_ERROR;
    }

#if defined(MBEDTLS_SELF_TEST)
    int verbose;
    verbose = Parameter_List[1].Integer_Value;
#endif

    if (0 == strcmp("all", Parameter_List[0].String_Value)) {
#if defined(MBEDTLS_SELF_TEST)
        QCLI_Printf(QCC_PRINTF_HANDLE, "AES Test\n");

        if (mbedtls_aes_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        QCLI_Printf(QCC_PRINTF_HANDLE, "AES-CCM Test\n");

        if (mbedtls_ccm_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        QCLI_Printf(QCC_PRINTF_HANDLE, "SHA1 Test\n");

        if (mbedtls_sha1_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        QCLI_Printf(QCC_PRINTF_HANDLE, "SHA256 Test\n");

        if (mbedtls_sha256_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");
#endif

        return QAPI_OK;
    }
#if defined(MBEDTLS_SELF_TEST)
    else if (0 == strcmp("aes", Parameter_List[0].String_Value)) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "AES Test\n");

        if (mbedtls_aes_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("ccm", Parameter_List[0].String_Value)) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "AES-CCM Test\n");

        if (mbedtls_ccm_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("sha1", Parameter_List[0].String_Value)) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "SHA1 Test\n");

        if (mbedtls_sha1_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("sha256", Parameter_List[0].String_Value)) {
        QCLI_Printf(QCC_PRINTF_HANDLE, "SHA256 Test\n");

        if (mbedtls_sha256_self_test(verbose) != 0)
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(QCC_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    }
#endif

    else {
        QCLI_Printf(QCC_PRINTF_HANDLE, "Invalid test module\n");
        return QAPI_ERROR;
    }

    return QAPI_OK;
}
