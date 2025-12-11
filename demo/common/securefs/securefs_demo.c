/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "securefs_demo.h"
#include "CeML.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "dirent.h"
#include "fcntl.h"
#include "fs.h"
#include "fw_upgrade_mem.h"
#include "littlefs_fs.h"
#include "qapi_firmware_upgrade.h"
#include "qapi_flash.h"
#include "qapi_status.h"
#include "qcli_api.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef off_t
#define off_t int32_t
#endif

#define USER_PASSWORD_SIZE 16
#define META_DATA_SIZE 64         // for Secure Storage tests
#define RANDOMTEST_OPERATIONS 100 // number of operations to run in random test
#define MAX_WRITE_BUFFER_SIZE (20 * 1024) // 20K limit

/*
 * This variable is a password that is used for SecureFs when the explicit
 * password is not specified.
 */
uint8_t g_securefs_password[USER_PASSWORD_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

typedef struct g_securefs_demo_test_s {
  off_t g_securefs_demo_off_t;
  size_t g_securefs_demo_size_t;
} g_securefs_demo_test_t;

typedef struct securefs_visible_header_s {
  uint32_t version;
  uint32_t hmac_type;
  uint32_t cipher_type;
  uint32_t plaintext_size;
  uint8_t reserved[16];
} securefs_visible_header_t;

g_securefs_demo_test_t g_securefs_demo_test;
extern struct fs_mount_t *mp;

/*
 * This file contains the command handlers for secure storage management
 * operations on non-volatile memory like list, delete, read, write
 *
 */

QCLI_Group_Handle_t qcli_securefs_handle; /* Handle for Fs Command Group. */

#define SECUREFS_DEMO_PRINTF(...) QCLI_Printf(qcli_securefs_handle, __VA_ARGS__)

#define hex_to_dec_nibble(hex_nibble)                                          \
  ((hex_nibble >= 'a')                                                         \
       ? (hex_nibble - 'a' + 10)                                               \
       : ((hex_nibble >= 'A') ? (hex_nibble - 'A' + 10) : (hex_nibble - '0')))

void *g_securefs_demo_ctxt;

int securefs_helper_initialize_and_write_visible_header(const char *name) {
  int status;
  securefs_visible_header_t securefs_visible_header;
  memset(&securefs_visible_header, 0, sizeof(securefs_visible_header));
  securefs_visible_header.version = CURRENT_VISIBLE_HEADER_VERSION;
  securefs_visible_header.hmac_type = SECUREFS_HMAC_TYPE_SHA256;
  securefs_visible_header.cipher_type = SECUREFS_CIPHER_TYPE_AES128_CBC;
  securefs_visible_header.plaintext_size = 0;

  status = write_func(name, 0, &securefs_visible_header,
                      sizeof(securefs_visible_header));

  return status;
}

int securefs_helper_read_check_and_parse_visible_header(
    const char *name, securefs_visible_header_t *ctxt) {
  int status;
  if (NULL == ctxt) {
    return -1;
  }

  securefs_visible_header_t securefs_visible_header;
  memset(&securefs_visible_header, 0, sizeof(securefs_visible_header));

  int file = open(name, O_RDONLY, 0);

  if (file == -1) {
    printf("Error opening file:%s.\n", name);
    return -1;
  }

  lseek(file, 0, SEEK_SET);

  int len_read = 0;
  len_read =
      read(file, &securefs_visible_header, sizeof(securefs_visible_header));

  ctxt->hmac_type = securefs_visible_header.hmac_type;
  ctxt->cipher_type = securefs_visible_header.cipher_type;
  ctxt->plaintext_size = securefs_visible_header.plaintext_size;

  close(file);
  return 0;
}

static uint32_t SecureStorageTest_Random(uint32_t length) {
  uint32_t input_data_len = length;
  uint32_t output_data_len = length;
  uint8_t *input_data = NULL;
  uint8_t *input_data1 = NULL;
  uint8_t *output_data = NULL;
  uint32_t i = 0;
  int errors = 0;
  uint32_t meta_data[META_DATA_SIZE];
  uint32_t meta_data_len = META_DATA_SIZE;
  CeMLErrorType ret_val = CEML_ERROR_FAILURE;

  input_data = (uint8_t *)malloc(length);
  input_data1 = (uint8_t *)malloc(length);
  if (!input_data) {
    printf("SecureStorage malloc for input buffer failed\n");
    errors++;
    goto end;
  }

  output_data = (uint8_t *)malloc(length);
  if (!output_data) {
    printf("SecureStorage malloc for output buffer failed\n");
    errors++;
    goto end;
  }

  for (i = 0; i < length; i++) {
    *(input_data + i) = (i % 0xFF); // max value in uint8_t is 0xFF
  }

  // meta data at the end
  if (CEML_ERROR_SUCCESS !=
      (ret_val = secure_storage_encrypt_authenticate(
           g_securefs_password, USER_PASSWORD_SIZE, input_data, input_data_len,
           (void *)output_data, &output_data_len, meta_data, meta_data_len))) {
    printf("SecureStorage Test Encryption failed: %d\n", ret_val);
    errors++;
    goto end;
  }

  if (CEML_ERROR_SUCCESS !=
      (ret_val = secure_storage_decrypt_authenticate(
           g_securefs_password, USER_PASSWORD_SIZE, (void *)output_data,
           output_data_len, input_data1, &input_data_len, meta_data,
           meta_data_len))) {
    printf("SecureStorage Test Decryption failed: %d\n", ret_val);
    errors++;
    goto end;
  }

  for (i = 0; i < length; i++) {
    if (*(input_data1 + i) != (i % 0xFF)) {
      errors++;
      printf("SecureStorage Test Comparision failed for index: %d\n", i);
      break;
    }
  }

end:
  if (input_data) {
    free(input_data);
    input_data = NULL;
  }

  if (output_data) {
    free(output_data);
    output_data = NULL;
  }
  printf("errors = %d\n", errors);

  return errors;
}

static qapi_Status_t
securefs_run_unittest(uint32_t Parameter_Count,
                      QAPI_Console_Parameter_t *Parameter_List) {
  qapi_Status_t status = QAPI_OK;

  // check parameters
  if (Parameter_Count < 1) {
    SECUREFS_DEMO_PRINTF("Invalid number of parameters\r\n");
    status = QAPI_ERR_INVALID_PARAM;
    goto securefs_demo_run_unittests_on_error;
  }

  if (!Parameter_List[0].Integer_Is_Valid) {
    SECUREFS_DEMO_PRINTF("number_of_unittests_to_run is not valid integer\r\n");
    status = QAPI_ERR_INVALID_PARAM;
    goto securefs_demo_run_unittests_on_error;
  }

  uint32_t length = Parameter_List[0].Integer_Value;

  int errors = SecureStorageTest_Random(length);

  if (errors) {
    SECUREFS_DEMO_PRINTF("unittests for secure storage failed\r\n");
    return status;
  }

securefs_demo_run_unittests_on_error:
  if (QAPI_OK != status) {
    SECUREFS_DEMO_PRINTF(
        "Usage: run_unittests number_of_unittests_to_run.\r\n");
    return QCLI_STATUS_ERROR_E;
  }

  return status;
}

static qapi_Status_t
securefs_set_password(uint32_t Parameter_Count,
                      QAPI_Console_Parameter_t *Parameter_List) {
  uint8_t user_password[USER_PASSWORD_SIZE];

  if (Parameter_Count < 1) {
    SECUREFS_DEMO_PRINTF("Invalid number of parameters\r\n");
    return QAPI_ERR_INVALID_PARAM;
  }

  char *password_in_hex = Parameter_List[0].String_Value;
  int status_code = convert_data_in_hex_to_byte_array(
      password_in_hex, user_password, USER_PASSWORD_SIZE);
  if (0 != status_code) {
    SECUREFS_DEMO_PRINTF(
        "Invalid password_in_hex, must be exactly 32 hex chars\r\n");
    return QAPI_ERR_INVALID_PARAM;
  } else {
    memcpy(g_securefs_password, user_password, USER_PASSWORD_SIZE);
    SECUREFS_DEMO_PRINTF("Set user password success, total 32 hex chars\r\n",
                         password_in_hex);
  }
}

static qapi_Status_t securefs_mkdir(uint32_t Parameter_Count,
                                    QAPI_Console_Parameter_t *Parameter_List) {
  char *name = NULL;
  int err = 0;

  if (Parameter_Count < 1 || !Parameter_List) {
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  name = Parameter_List[0].String_Value;

  err = vfs_mkdir(name);
  if (err == 0)
    printf("vfs_mkdir:%s successfully.\r\n", "/lfs/dir1");
  else if (err == -EEXIST)
    printf("vfs_mkdir:%s exists.\r\n", "/lfs/dir1");
  else
    printf("vfs_mkdir:%s failed.(%d).\r\n", "/lfs/dir1", err);

  return QAPI_OK;
}

static qapi_Status_t securefs_rm(uint32_t Parameter_Count,
                                 QAPI_Console_Parameter_t *Parameter_List) {
  char *name = NULL;

  if (Parameter_Count < 1 || !Parameter_List) {
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  name = Parameter_List[0].String_Value;

  rm_func(name);

  return QAPI_OK;
}

static qapi_Status_t securefs_ls(uint32_t Parameter_Count,
                                 QAPI_Console_Parameter_t *Parameter_List) {
  char *name = NULL;

  if (Parameter_Count < 1 || !Parameter_List) {
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  name = Parameter_List[0].String_Value;

  ls_func(name);

  return QAPI_OK;
}

int securefs_write_func(const char *name, const uint8_t *buf) {
  uint32_t len = 0;
  uint32_t len_byte = 0;
  int i = 0;
  uint8_t value = 0;
  CeMLErrorType ret_val = CEML_ERROR_FAILURE;
  uint32_t meta_data[META_DATA_SIZE];
  uint32_t meta_data_len = META_DATA_SIZE;
  uint8_t *input_data = NULL;
  uint8_t *output_data = NULL;
  uint32_t input_data_len = 0;
  uint32_t output_data_len = 0;

  if (is_fs_mounted() == 0) {
    printf("FS is not mounted, please mount FS first.\r\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  len = strlen((char *)buf);
  if (len % 2 != 0) {
    printf("The length of the hex string is %d, make sure to input even number "
           "of hex char.\r\n",
           len);
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  len_byte = len / 2;
  if (len_byte % AES_128_BLOCK_SIZE == 0) {
    input_data_len = len_byte;
  } else {
    input_data_len =
        len_byte + (AES_128_BLOCK_SIZE - (len_byte % AES_128_BLOCK_SIZE));
  }
  output_data_len = input_data_len;

  // check hex
  for (i = 0; i < (int)len; i++) {
    if (!isxdigit((int)buf[i])) {
      printf("hex data in hex, please enter [0-9] or [A-F]\r\n");
      return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
  }

  input_data = (uint8_t *)malloc(input_data_len);

  if (!input_data) {
    printf("SecureStorage malloc for input buffer failed\n");
    goto end;
  }
  memset(input_data, 0, input_data_len);

  output_data = (uint8_t *)malloc(output_data_len);
  if (!output_data) {
    printf("SecureStorage malloc for output buffer failed\n");
    goto end;
  }
  memset(output_data, 0, output_data_len);

  for (i = 0; i < (int)len; i += 2) {
    value = hex_to_byte(buf + i);
    input_data[i / 2] = value;
  }

  if (CEML_ERROR_SUCCESS !=
      (ret_val = secure_storage_encrypt_authenticate(
           g_securefs_password, USER_PASSWORD_SIZE, input_data, input_data_len,
           (void *)output_data, &output_data_len, meta_data, meta_data_len))) {
    printf("SecureStorage Test Decryption failed: %d\n", ret_val);
    goto end;
  }

  /* write the header */
  securefs_visible_header_t securefs_visible_header;
  memset(&securefs_visible_header, 0, sizeof(securefs_visible_header));
  securefs_visible_header.version = CURRENT_VISIBLE_HEADER_VERSION;
  securefs_visible_header.hmac_type = SECUREFS_HMAC_TYPE_SHA256;
  securefs_visible_header.cipher_type = SECUREFS_CIPHER_TYPE_AES128_CBC;
  securefs_visible_header.plaintext_size = len_byte;

  write_func(name, 0, &securefs_visible_header,
             sizeof(securefs_visible_header));

  /* write the meta data */
  write_func(name, sizeof(securefs_visible_header), &meta_data, META_DATA_SIZE);

  /* write the encrypted data */
  write_func(name, sizeof(securefs_visible_header) + META_DATA_SIZE,
             output_data, output_data_len);

end:
  if (input_data) {
    free(input_data);
    input_data = NULL;
  }

  if (output_data) {
    free(output_data);
    output_data = NULL;
  }

  return 0;
}

static qapi_Status_t securefs_write(uint32_t Parameter_Count,
                                    QAPI_Console_Parameter_t *Parameter_List) {
  char *name = NULL;
  int offset = 0;
  uint8_t *buf = NULL;
  uint8_t *dup_hex = NULL;
  bool should_free_buf = false;

  if ((Parameter_Count != 2 && Parameter_Count != 3) || !Parameter_List) {
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  if (Parameter_Count == 2) {
    name = Parameter_List[0].String_Value;
    buf = (uint8_t *)Parameter_List[1].String_Value;
  } else if (Parameter_Count == 3) {
    if (!Parameter_List[2].Integer_Is_Valid) {
      printf("Invalid buffer size parameter\n");
      return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    dup_hex = (uint8_t *)Parameter_List[1].String_Value;

    if (!dup_hex || !isxdigit((int)dup_hex[0]) || (strlen(dup_hex) != 1)) {
      printf("hex data in hex, please enter [0-9] or [A-F]\r\n");
      return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[2].Integer_Value <= 0 ||
        Parameter_List[2].Integer_Value > MAX_WRITE_BUFFER_SIZE) {
      printf("Invalid buffer size. Must be between 1 and %d\n",
             MAX_WRITE_BUFFER_SIZE);
      return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    name = Parameter_List[0].String_Value;
    buf = (uint8_t *)malloc(Parameter_List[2].Integer_Value + 1);

    if (!buf) {
      printf("fail to allocate memory\n");
      return QAPI_ERROR;
    }
    should_free_buf = true;

    for (int i = 0; i < Parameter_List[2].Integer_Value; i++) {
      buf[i] = dup_hex[0];
    }
    buf[Parameter_List[2].Integer_Value] = '\0';
  }

  securefs_write_func(name, buf);

  if (should_free_buf) {
    free(buf);
  }

  return QAPI_OK;
}

int sucurefs_read_func(const char *name, size_t len_read) {
  securefs_visible_header_t securefs_visible_header;
  uint32_t input_data_len = 0;
  uint32_t output_data_len = 0;
  uint32_t plain_data_len = 0;
  uint8_t *input_data = NULL;
  uint8_t *output_data = NULL;
  int errors = 0;
  uint32_t meta_data[META_DATA_SIZE];
  uint32_t meta_data_len = META_DATA_SIZE;
  CeMLErrorType ret_val = CEML_ERROR_FAILURE;
  if (is_fs_mounted() == 0) {
    printf("FS is not mounted, please mount FS first.\r\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  memset(&securefs_visible_header, 0, sizeof(securefs_visible_header));

  int file = open(name, O_RDONLY, 0);
  if (file == -1) {
    printf("Error opening file:%s.\n", name);
    return -1;
  }

  /* read the header */
  lseek(file, 0, SEEK_SET);
  read(file, &securefs_visible_header, sizeof(securefs_visible_header));
  plain_data_len = securefs_visible_header.plaintext_size;

  if (plain_data_len % AES_128_BLOCK_SIZE == 0) {
    input_data_len = plain_data_len;
  } else {
    input_data_len = plain_data_len + (AES_128_BLOCK_SIZE -
                                       (plain_data_len % AES_128_BLOCK_SIZE));
  }
  output_data_len = input_data_len;

  input_data = (uint8_t *)malloc(input_data_len);
  if (!input_data) {
    printf("SecureStorage malloc for input buffer failed\n");
    goto end;
  }

  output_data = (uint8_t *)malloc(output_data_len);
  if (!output_data) {
    printf("SecureStorage malloc for output buffer failed\n");
    goto end;
  }

  /* read the meta data */
  lseek(file, sizeof(securefs_visible_header), SEEK_SET);
  read(file, meta_data, META_DATA_SIZE);

  /* read the encrypted data */
  lseek(file, sizeof(securefs_visible_header) + META_DATA_SIZE, SEEK_SET);
  read(file, input_data, input_data_len);

  if (CEML_ERROR_SUCCESS !=
      (ret_val = secure_storage_decrypt_authenticate(
           g_securefs_password, USER_PASSWORD_SIZE, (void *)input_data,
           input_data_len, output_data, &output_data_len, meta_data,
           meta_data_len))) {
    printf("SecureStorage Test Decryption failed: %d\n", ret_val);
    goto end;
  }

  if (len_read > 0) {
    printf("0X");
    if (len_read > plain_data_len) {
      for (int i = 0; i < plain_data_len; i++) {
        printf("%02X", (unsigned char)output_data[i]);
      }
      printf("\r\nread length exceeds plain data length\n");
    } else {
      for (int i = 0; i < len_read; i++) {
        printf("%02X", (unsigned char)output_data[i]);
      }
    }
    printf("\r\n");
  }

end:
  if (input_data) {
    free(input_data);
    input_data = NULL;
  }

  if (output_data) {
    free(output_data);
    output_data = NULL;
  }
  close(file);

  return 0;
}

static qapi_Status_t securefs_read(uint32_t Parameter_Count,
                                   QAPI_Console_Parameter_t *Parameter_List) {
  char *path = NULL;
  int len_read = 0;

  if (Parameter_Count != 2 || !Parameter_List) {
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  if (!Parameter_List[1].Integer_Is_Valid) {
    printf("Invalid buffer size parameter\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
  }

  path = Parameter_List[0].String_Value;
  len_read = Parameter_List[1].Integer_Value;

  sucurefs_read_func(path, len_read);

  return QAPI_OK;
}

const QAPI_Console_Command_t securefs_cmd_list[] = {
    // cmd_function         cmd_string      usage_string        description
    {securefs_set_password, "setPassword", "<password_in_hex>",
     "set password for securefs"},
    {securefs_ls, "ls", "", "lists valid securefs files"},
    {securefs_read, "read", "/path <length>",
     "reads and decrypts length bytes of data from the opened file and prints "
     "it as hex"},
    {securefs_write, "write",
     "/path <hex_data> [num of consecutive hex bytes if hex_data consists of a "
     "single hex character]",
     "encrypts the hex data and writes it into opened file"},
    {securefs_rm, "rm", "/path", "remove file or empty folder"},
    //{securefs_run_unittest, "run_unittest", "<number_of_unittests_to_run>"},
};

const QAPI_Console_Command_Group_t securefs_cmd_group = {
    "SecureFS", /* Group_String: will display cmd prompt as "SecureFs> " */
    sizeof(securefs_cmd_list) /
        sizeof(securefs_cmd_list[0]), /* Command_Count */
    securefs_cmd_list                 /* Command_List */
};

/*****************************************************************************
 * This function is used to register the Fs Command Group with QCLI.
 *****************************************************************************/
void Initialize_SecureFs_Demo(void) {
  /* Attempt to reqister the Command Groups with the qcli framework.*/
  qcli_securefs_handle = QCLI_Register_Command_Group(NULL, &securefs_cmd_group);
  if (qcli_securefs_handle) {
    QCLI_Printf(qcli_securefs_handle, "SecureFs Registered\n");
  }

  return;
}