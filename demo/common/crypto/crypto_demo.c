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
static qapi_Status_t Command_Kdf_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Kdf_Crypto_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t Crypto_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {Command_Unit_Test, "unittest", "\n\nunittest <Module> <verbose>", "Run Crypto Unittest"},
    {Command_Kdf_Test, "kdftest", "\n\nkdftest <user_input>", "Run Kdf test"},
    {Command_Kdf_Crypto_Test, "kdfcryptotest", "\n\nkdfcryptotest <kdf_key_input> <sw_key> [plaintext_length]", "Run KDF Crypto test"},
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

#define hex_to_dec_nibble(hex_nibble) ( \
    ((hex_nibble >= '0' && hex_nibble <= '9') ? (hex_nibble - '0') : \
    (hex_nibble >= 'a' && hex_nibble <= 'f') ? (hex_nibble - 'a' + 10) : \
    (hex_nibble >= 'A' && hex_nibble <= 'F') ? (hex_nibble - 'A' + 10) : -1) )

int convert_data_in_hex_to_byte_array(const char * data_in_hex, uint8_t * data_as_byte_array, uint32_t data_as_byte_array_size)
{
    uint32_t i;
    const uint32_t data_in_hex_length = strlen(data_in_hex);

    if (data_as_byte_array_size > (UINT32_MAX / 2)) {
        return -1;
    }

    if ( (2*data_as_byte_array_size) != data_in_hex_length ) {
        return -1;
    }

    for ( i = 0; i < data_as_byte_array_size; i++ ) {
        int high_nibble = hex_to_dec_nibble(data_in_hex[2*i]);
        int low_nibble = hex_to_dec_nibble(data_in_hex[2*i+1]);

        if (high_nibble < 0 || low_nibble < 0) {
            return -1;  // Invalid hex character
        }

        data_as_byte_array[i] = (high_nibble << 4) | low_nibble;
    }

    return 0;
}

void reverse_uint8_array(uint8_t *array, int n)
{
    uint8_t tmp = 0, idx = 0;
    for(idx = 0;idx < n/2;idx++){
        tmp = array[idx];
        array[idx] = array[n-1-idx];
        array[n-1-idx] = tmp;
    }
}

void print_uint8_array(uint8_t *array, int n)
{
    int idx = 0;
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "[%d]:",n);
    for(idx = 0;idx < n;idx++){
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "%02x ",array[idx]);
    }
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "\r\n");
}

static qapi_Status_t Command_Kdf_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t user_input[16] = {0};
    uint64_t password = 0;
    uint32_t result[4] = {0};
    uint32_t result_len_in_words = 4;

    if (Parameter_Count < 1) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid number of arguments\r\n");
        goto crypto_demo_kdf_test_on_error;
    }

    if(0 != convert_data_in_hex_to_byte_array(Parameter_List[0].String_Value, user_input, 16)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid user_input_in_hex, must be exactly 32 hex chars\r\n");
        goto crypto_demo_kdf_test_on_error;
    }

    reverse_uint8_array(user_input, 16);

    if(0 != CeML_hw_kdf(0, 4, user_input, 16, password, result, result_len_in_words)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "KDF Test Fail\r\n");
        return QAPI_ERROR;
    }

    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "KDF Test Pass\r\n");
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "KDF output: %08x %08x %08x %08x \r\n", result[3], result[2], result[1], result[0]);
    return QAPI_OK;

crypto_demo_kdf_test_on_error:
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Usage: kdftest <user_input>\r\n");

    return QAPI_OK;
}

static qapi_Status_t Command_Kdf_Crypto_Test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t kdf_key[16] = {0};
    uint8_t sw_key[16] = {0};
    uint8_t iv[16] = {0};
    uint32_t decrypt_len = 16;
    uint32_t encrypt_len;
    uint8_t *input = NULL;
    uint8_t *encrypt_kdf = NULL;
    uint8_t *encrypt_sw = NULL;
    uint8_t *decrypt_kdf = NULL;
    uint8_t *data_test = NULL;
    qapi_Status_t ret = QAPI_ERROR;

    if (Parameter_Count < 2) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid number of arguments\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }

    if(0 != convert_data_in_hex_to_byte_array(Parameter_List[0].String_Value, kdf_key, 16)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid kdf_key_in_hex, must be exactly 32 hex chars\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }

    reverse_uint8_array(kdf_key, 16);

    if(0 != convert_data_in_hex_to_byte_array(Parameter_List[1].String_Value, sw_key, 16)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid sw_key_in_hex, must be exactly 32 hex chars\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }

    reverse_uint8_array(sw_key, 16);

    if(Parameter_Count > 2 && Parameter_List[2].Integer_Is_Valid){
        decrypt_len = Parameter_List[2].Integer_Value;
        if(decrypt_len % 16) {
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid input data length, must be multiple of 16\r\n");
            goto crypto_demo_kdf_crypto_test_on_error;
        }
        if(decrypt_len == 0 || decrypt_len > 4096) {
            QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Invalid input data length, must be between 16 and 4096\r\n");
            goto crypto_demo_kdf_crypto_test_on_error;
        }
    }

    encrypt_len = decrypt_len + 16;
    data_test = (uint8_t*) malloc (2*decrypt_len + 2*encrypt_len);
    if(!data_test) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "malloc for data of test fail\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }

    input = data_test;
    encrypt_kdf = data_test + decrypt_len;
    encrypt_sw = encrypt_kdf + encrypt_len;
    decrypt_kdf = encrypt_sw + encrypt_len;

    if (!nt_wlan_hw_prng_get(input, decrypt_len)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "get random input %d bytes fail\r\n", decrypt_len);
        goto crypto_demo_kdf_crypto_test_on_error;
    }
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Input ");
    print_uint8_array(input, decrypt_len);	

    /* get IV from prng */
    if (!nt_wlan_hw_prng_get(iv, 16))
    {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "get random iv fail\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }
	/* encrypt data with kdf key */
    if(0 != (ret = CeML_util_encrypt_with_key(0,kdf_key,16,iv,16,input,decrypt_len,encrypt_kdf,&encrypt_len))) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "encrypt data with kdf key fail %d\r\n", ret);
        goto crypto_demo_kdf_crypto_test_on_error;
    }
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Encrypt Data with KDF Key ");
    print_uint8_array(encrypt_kdf, encrypt_len);

    /* encrypt data with sw key */
    if(0 != (ret = CeML_util_encrypt_with_key(1,sw_key,16,iv,16,input,decrypt_len,encrypt_sw,&encrypt_len))) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "encrypt data with sw key fail %d\r\n", ret);
        goto crypto_demo_kdf_crypto_test_on_error;
    }
    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Encrypt Data with SW Key ");
    print_uint8_array(encrypt_sw, encrypt_len);

    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "The encrypt result with KDF key and SW key in the test ");
    if(memcmp(encrypt_kdf,encrypt_sw,encrypt_len)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "not match\r\n");
    }
    else {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "match\r\n");
    }

    /* decrypt data with kdf key */
    if(0 != CeML_util_decrypt_with_key(0,kdf_key,16,encrypt_kdf,encrypt_len,decrypt_kdf,&decrypt_len)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "decrypt data with kdf key fail\r\n");
        goto crypto_demo_kdf_crypto_test_on_error;
    }

    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "Decrypt Data with KDF Key ");
    print_uint8_array(decrypt_kdf, decrypt_len);

    QCLI_Printf(CRYPTO_PRINTF_HANDLE, "The encrypt and decrypt with kdf key in the test  ");
    if(memcmp(decrypt_kdf,input,decrypt_len)) {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "fail\r\n");
    }
    else {
        QCLI_Printf(CRYPTO_PRINTF_HANDLE, "success\r\n");
    }
    ret = QAPI_OK;
	
crypto_demo_kdf_crypto_test_on_error:
    if(data_test) {
        free(data_test);
    }
    return ret;
}