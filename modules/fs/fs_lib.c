/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "stdio.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "qapi_flash.h"
#include "qapi_firmware_upgrade.h"
#include "fw_upgrade_mem.h"
#include "littlefs_fs.h"
#include "fs.h"
#include <unistd.h>
#include "fcntl.h"
#include "dirent.h"
#include "qapi_status.h"
#include "qapi_console.h"
#include "qcli_api.h"

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

#define FS_SHELL_GROUP_NAME "FS"

void ls_func(const char *path)
{
    int rc;
    struct fs_dir_t dir;
    struct fs_dirent entry;
    fs_dir_t_init(&dir);
    rc = vfs_opendir(&dir, path);
    if (rc != 0) {
        printf("[ls_func] Failed to open directory %s\r\n", path);
        return;
    }

    for (;;) {
        rc = vfs_readdir(&dir, &entry);

        if (rc < 0) {
            printf("[ls_func] %s reading dir [%d]\r\n", __func__, rc);
            break;
        }

        if (entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            printf("[DIR] %s/%s\r\n", path, entry.name);
            char subdir[256];
            snprintf(subdir, sizeof(subdir), "%s/%s", path, entry.name);
        } else {
            printf("[FILE] %s/%s (size=%d)\r\n", path, entry.name, entry.size);
        }
    }

    vfs_closedir(&dir);
}

int read_func(const char *name, off_t offset, size_t len)
{
    int file = open(name, O_RDONLY, 0);
    if (file == -1) {
        printf("Error opening file:%s.\n", name);
        return -1;
    }
    printf("open %s for read\r\n", name);
    char *buf = (char *)malloc(len);
    if (buf == NULL) {
        printf("ERROR: no enough memory\r\n");
        return 0;
    }
    lseek(file, offset, SEEK_SET);

    int len_read = 0;
    len_read = read(file, buf, len);
    if (len_read > 0) {
        printf("0X");
        int i = 0;
        for (; i < (int)len_read; i++) {
            printf("%02X", (unsigned char)buf[i]);
        }
        printf("\r\n");
    } else if (len_read == 0) {
        printf("Fail to read from: %s, %d\r\n", name, len_read);
    } else {
        printf("Fail to read from: %s, %d\r\n", name, len_read);
    }

    free(buf);
    close(file);

    return 0;
}

int write_func(const char *name, off_t offset, const void *buf, size_t sz)
{
    int file = open(name, O_RDWR | O_CREAT, 0);
    if (file == -1) {
        printf("Error opening/creating file:%s.\n", name);
        return -1;
    }

    lseek(file, offset, SEEK_SET);
    write(file, buf, sz);
    close(file);
    return 0;
}

int rm_func(const char *name)
{
    int ret;
    ret = unlink(name);
    if (ret == 0) {
        printf("%s removed.\r\n", name);
    } else {
        printf("Failed to remove %s:%d\r\n", name, ret);
    }
    return ret;
}

int mount_func(void)
{
    extern int init_fs(void);
    init_fs();
    return 0;
}

static qapi_Status_t Mount(uint32_t __attribute__((__unused__)) Parameter_Count,
                           QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    mount_func();

    return QAPI_OK;
}

static qapi_Status_t Rm(uint32_t __attribute__((__unused__)) Parameter_Count,
                        QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    char *name = NULL;

    if (Parameter_Count < 1 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    name = Parameter_List[0].String_Value;

    rm_func(name);

    return QAPI_OK;
}

static qapi_Status_t Ls(uint32_t __attribute__((__unused__)) Parameter_Count,
                        QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    char *name = NULL;

    if (Parameter_Count < 1 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    name = Parameter_List[0].String_Value;

    ls_func(name);

    return QAPI_OK;
}

uint8_t hex_to_byte(uint8_t *hex)
{
    uint8_t byte = 0;

    if ('0' <= hex[0] && hex[0] <= '9') {
        byte = hex[0] - '0';
    } else if ('a' <= hex[0] && hex[0] <= 'f') {
        byte = hex[0] - 'a' + 10;
    } else if ('A' <= hex[0] && hex[0] <= 'F') {
        byte = hex[0] - 'A' + 10;
    } else {
        printf("Invalid hexadecimal character: %c\r\n", hex[0]);
        return 0;
    }

    byte <<= 4;
    if ('0' <= hex[1] && hex[1] <= '9') {
        byte += hex[1] - '0';
    } else if ('a' <= hex[1] && hex[1] <= 'f') {
        byte += hex[1] - 'a' + 10;
    } else if ('A' <= hex[1] && hex[1] <= 'F') {
        byte += hex[1] - 'A' + 10;
    } else {
        printf("Invalid hexadecimal character: %c\r\n", hex[1]);
        return 0;
    }

    return byte;
}

static qapi_Status_t Write(uint32_t __attribute__((__unused__)) Parameter_Count,
                           QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    char *name = NULL;
    int offset = 0;
    uint8_t *buf = NULL;
    // int sz = 0;
    size_t len = 0;
    int i = 0;
    uint8_t value = 0;

    if (is_fs_mounted() == 0) {
        printf("FS is not mounted, please mount FS first.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_Count != 3 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    name = Parameter_List[0].String_Value;
    offset = Parameter_List[1].Integer_Value;
    buf = (uint8_t *)Parameter_List[2].String_Value;
    len = strlen((char *)buf);
    if (len % 2 != 0) {
        printf("The length of the hex string is %d, make sure to input even number of hex char.\r\n", len);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    // check hex
    for (i = 0; i < (int)len; i++) {
        if (!isxdigit((int)buf[i])) {
            printf("hex data in hex, please enter [0-9] or [A-F]\r\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
    }

    for (i = 0; i < (int)len; i += 2) {
        value = hex_to_byte(buf + i);
        write_func(name, (off_t)(offset + i / 2), &value, 1);
    }

    return QAPI_OK;
}

static qapi_Status_t Read(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    char *path = NULL;
    int offset = 0;
    int len = 0;

    if (is_fs_mounted() == 0) {
        printf("FS is not mounted, please mount FS first.\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_Count != 3 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    path = Parameter_List[0].String_Value;
    offset = Parameter_List[1].Integer_Value;
    len = Parameter_List[2].Integer_Value;

    read_func(path, (off_t)offset, (size_t)len);
    return QAPI_OK;
}
const QAPI_Console_Command_t fs_shell_cmds[] = {
    // cmd_function      cmd_string      usage_string                            description
    {Ls, "ls", " /path", "list directory contents, path should include the mount point."},
    {Read, "read", " /path <offset> <length>", "read from file"},
    {Write, "write", " /path <offset> <hex data>", "write data to file, hex data should be even number hex char"},
    {Rm, "rm", " /path", "Remove file or empty folder"},
    {Mount, "mount", " ", "mount the FS if not mounted"},

};
const QAPI_Console_Command_Group_t fs_shell_cmd_group = {
    FS_SHELL_GROUP_NAME, sizeof(fs_shell_cmds) / sizeof(QAPI_Console_Command_t), fs_shell_cmds};

QAPI_Console_Group_Handle_t fs_shell_cmd_group_handle;

void fs_shell_init(void)
{
    fs_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &fs_shell_cmd_group);
}
