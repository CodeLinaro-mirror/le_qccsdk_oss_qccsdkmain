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
/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define CRYPTO_PRINTF_HANDLE qcli_crypto_group

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
QAPI_Console_Group_Handle_t qcli_crypto_group; /* Handle for our QCLI Command Group. */

/**********************************************************************************************************/
/* Function Declarations											                                      */
/**********************************************************************************************************/
static qapi_Status_t Command_Unit_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t Crypto_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {Command_Unit_Test, "unittest", "\n\nunittest <Module> <verbose>", "Run Crypto Unittest"},
};

const QAPI_Console_Command_Group_t Crypto_Command_Group = {
    "Crypto", /* Firmware Upgrade */
    sizeof(Crypto_Command_List) / sizeof(QAPI_Console_Command_t),
    Crypto_Command_List,
};

/**********************************************************************************************************/
/* Function Definitions    											                                      */
/**********************************************************************************************************/
/* This function is used to register the Firmware Upgrade Command Group with QCLI   */
void Initialize_Crypto_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    CRYPTO_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &Crypto_Command_Group);
    if (CRYPTO_PRINTF_HANDLE) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Crypto Registered \n");
    }
}

/*=================================================================================================*/

/**
   @brief This function processes the "unittest" command from the CLI.
*/

static qapi_Status_t Command_Unit_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    int verbose;

    if (Parameter_Count != 2) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "usage: unittest <Module> <verbose>\n");
        return QAPI_ERROR;
    }

    verbose = Parameter_List[1].Integer_Value;

#if !CONFIG_MBEDTLS_PKA_TEST
    (void)verbose;
#endif

    if (0 == strcmp("all", Parameter_List[0].String_Value)) {
#if CONFIG_MBEDTLS_PKA_TEST
        /* MPI Test */
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "MPI Test\n");

        if (mbedtls_mpi_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n\n");

        /* RSA Test */
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "RSA Test\n");

        if (mbedtls_rsa_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n\n");

        /* DH Test */
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "DH Test\n");

        if (mbedtls_dhm_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n\n");

        /* ECP Test */
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "ECP Test\n");

        if (mbedtls_ecp_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n\n");

        /* ECDSA Test */
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "ECDSA Test\n");

        if (mbedtls_ecdsa_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n\n");
#endif
        return QAPI_OK;
    }
#if CONFIG_MBEDTLS_PKA_TEST
    else if (0 == strcmp("mpi", Parameter_List[0].String_Value)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "MPI Test\n");

        if (mbedtls_mpi_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("rsa", Parameter_List[0].String_Value)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "RSA Test\n");

        if (mbedtls_rsa_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("dh", Parameter_List[0].String_Value)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "DH Test\n");

        if (mbedtls_dhm_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("ecp", Parameter_List[0].String_Value)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "ECP Test\n");

        if (mbedtls_ecp_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    } else if (0 == strcmp("ecdsa", Parameter_List[0].String_Value)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "ECDSA Test\n");

        if (mbedtls_ecdsa_pka_self_test(verbose) != 0)
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Fail\n");
        else
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Test Pass\n");

        return QAPI_OK;
    }
#endif

    else {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid test module\n");
        return QAPI_ERROR;
    }

    return QAPI_OK;
}
