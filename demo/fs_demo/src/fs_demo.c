/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
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

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

extern struct fs_mount_t *mp;

#define VFS_API_DEMO
#define VFS_BOOT_COUNT_TEST
#define VFS_DIR_TEST

#undef VFS_API_DEMO
#define POSIX_FS_TEST

void list_dir(const char *path, int indent)
{
    // printf("list_dir %s\r\n", path);
    int rc;
    struct fs_dir_t dir;
    struct fs_dirent entry;
    fs_dir_t_init(&dir);
    rc = vfs_opendir(&dir, path);
    int i;
    if (rc != 0) {
        printf("[list_dir] Failed to open directory %s\r\n", path);
        return;
    }

    for (;;) {
        rc = vfs_readdir(&dir, &entry);

        if (rc < 0) {
            printf("[list_dir] %s reading dir [%d]\r\n", __func__, rc);
            break;
        }

        if (entry.name[0] == 0) {
            break;
        }

        for (i = 0; i < indent; i++) {
            printf(" ");
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            printf("[DIR] %s/%s\r\n", path, entry.name);
            char subdir[256];
            snprintf(subdir, sizeof(subdir), "%s/%s", path, entry.name);
            // list_dir(subdir, indent+2);
        } else {
            printf("[FILE] %s/%s (size=%d)\r\n", path, entry.name, entry.size);
        }
    }

    vfs_closedir(&dir);
}

void read_boot_count(void)
{
    int err = 0;

    struct fs_file_t file;
    fs_file_t_init(&file);
    char fname1[100];
    snprintf(fname1, sizeof(fname1), "%s/boot_count", mp->mnt_point);
    int boot_count = 0;

    // open file count
    err = vfs_open(&file, fname1, FS_O_CREATE | FS_O_RDWR);
    if (err < 0) {
        printf("FAIL: open %s: %d\r\n", fname1, err);
        return;
    }
    // read file
    err = vfs_read(&file, &boot_count, sizeof(boot_count));
    if (err < 0) {
        printf("FAIL: read %s: %d\r\n", fname1, err);
        goto out;
    }
    // write file
    boot_count += 1;
    printf("boot_count: %d\n", boot_count);
    err = vfs_seek(&file, 0, LFS_SEEK_SET);
    if (err < 0) {
        printf("FAIL: seek %s: %d\r\n", fname1, err);
        goto out;
    }
    err = vfs_write(&file, &boot_count, sizeof(boot_count));
    if (err < 0) {
        printf("FAIL: write %s: %d\r\n", fname1, err);
        goto out;
    }

out:
    vfs_close(&file);
}

void dir_test(void)
{
    int err = 0;
    struct fs_dir_t dir;
    fs_dir_t_init(&dir);
    printf("open /lfs\r\n");
    err = vfs_opendir(&dir, "/lfs");
    if (err < 0) {
        printf("FAIL: opendir %s: %d\r\n", "/lfs", err);
        return;
    }

    struct fs_dir_t dir1;
    fs_dir_t_init(&dir1);
    printf("open /lfs/dir1\r\n");
    err = vfs_opendir(&dir1, "/lfs/dir1");
    if (err < 0) {
        printf("FAIL: opendir %s: %d\r\n", "/lfs/dir1", err);
        goto out1;
    }

    struct fs_dir_t dir2;
    fs_dir_t_init(&dir2);
    printf("open /lfs/dir1/dir11\r\n");
    err = vfs_opendir(&dir2, "/lfs/dir1/dir11");
    if (err < 0) {
        printf("FAIL: opendir %s: %d\r\n", "/lfs/dir1/dir11", err);
        goto out2;
    }

    struct fs_dir_t dir3;
    fs_dir_t_init(&dir3);
    printf("open /lfs/dir1/dir11\r\n");
    err = vfs_opendir(&dir3, "/lfs/dir1/dir11");
    if (err < 0) {
        printf("FAIL: opendir %s: %d\r\n", "/lfs/dir1/dir11", err);
        goto out3;
    }

    vfs_closedir(&dir3);

out3:
    vfs_closedir(&dir2);
out2:
    vfs_closedir(&dir1);
out1:
    vfs_closedir(&dir);
}

static void unmount_fs(void)
{
    return;
    int err = 0;

    err = vfs_unmount(mp);
    if (err == 0) {
        UART_SEND_DIRECT("vfs_unmount successfully.\r\n");
    } else {
        UART_SEND_DIRECT("vfs_unmount failed. \r\n");
    }
}

void vfs_api_test(void)
{
    int err = 0;

    // file test
#ifdef VFS_BOOT_COUNT_TEST
    read_boot_count();
#endif  // VFS_BOOT_COUNT_TEST

    // dir test
    // create a new dir under the root /lfs
    err = vfs_mkdir("/lfs/dir1");
    if (err == 0)
        printf("vfs_mkdir:%s successfully.\r\n", "/lfs/dir1");
    else if (err == -EEXIST)
        printf("vfs_mkdir:%s exists.\r\n", "/lfs/dir1");
    else
        printf("vfs_mkdir:%s failed.(%d).\r\n", "/lfs/dir1", err);

    err = vfs_mkdir("/lfs/dir1/dir11");
    if (err == 0)
        printf("vfs_mkdir:%s successfully.\r\n", "/lfs/dir1/dir11");
    else if (err == -EEXIST)
        printf("vfs_mkdir:%s exists.\r\n", "/lfs/dir1/dir11");
    else
        printf("vfs_mkdir:%s failed.(%d).\r\n", "/lfs/dir1/dir11", err);

    list_dir("/lfs", 0);

#ifdef VFS_DIR_TEST
    dir_test();
#endif  // VFS_DIR_TEST
}

static void posix_fs_test(void)
{
    const char *filename = "/lfs/boot_count";

    int file = open(filename, O_RDWR | O_CREAT, 0);
    if (file == -1) {
        printf("Error opening/creating file:%s.\n", filename);
        return;
    }

    int count = 0;
    read(file, &count, sizeof(int));
    printf("Current count: %d\n", count);

    count++;
    lseek(file, 0, SEEK_SET);

    write(file, &count, sizeof(int));
    close(file);

    printf("Updated count: %d\n", count);

    ls_func("/lfs");

    read_func("/lfs/dir1/file1", 0, 10);

    ls_func("/lfs/dir1");

    int value = 0x12345678;
    write_func("/lfs/dir1/file1", 0, &value, sizeof(int));
    read_func("/lfs/dir1/file1", 0, 10);
    ls_func("/lfs/dir1");

    write_func("/lfs/dir1/file1", 4, &value, sizeof(int));
    read_func("/lfs/dir1/file1", 0, 10);
    ls_func("/lfs/dir1");

    rm_func("/lfs/dir1/file2");
    ls_func("/lfs/dir1");

    mount_func();
}

void app_init(void) {}

void app_main(void)
{
    UART_SEND_DIRECT("fs_demo running...\r\n");

#ifdef CONFIG_FILE_SYSTEM

#ifdef VFS_API_DEMO
    vfs_api_test();
#endif  // VFS_API_DEMO

#ifdef POSIX_FS_TEST
    posix_fs_test();
#endif  // POSIX_FS_TEST

    unmount_fs();
#endif  // CONFIG_FILE_SYSTEM

    UART_SEND_DIRECT("fs_demo finished\r\n");
    UART_SEND_DIRECT("app_main over\r\n");
}
