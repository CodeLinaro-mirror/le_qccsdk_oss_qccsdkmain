/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LITTLEFS_FS_H
#define LITTLEFS_FS_H

#include "lfs.h"
#include "semphr.h"
#include "fs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct fs_littlefs {
    /* Defaulted in driver, customizable before mount. */
    struct lfs_config *cfg;

    /* Must be cfg.cache_size */
    uint8_t *read_buffer;

    /* Must be cfg.cache_size */
    uint8_t *prog_buffer;

    /* Must be cfg.lookahead_size/4 elements, and
     * cfg.lookahead_size must be a multiple of 8.
     */
    uint32_t *lookahead_buffer[CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE / sizeof(uint32_t)];

    /* These structures are filled automatically at mount. */
    struct lfs lfs;
    void *backend;
    uint32_t id;
    SemaphoreHandle_t mutex;
};

#define FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name, read_sz, prog_sz, cache_sz, lookahead_sz) \
    static uint8_t __attribute__((aligned(8))) name##_read_buffer[cache_sz];              \
    static uint8_t __attribute__((aligned(8))) name##_prog_buffer[cache_sz];              \
    static uint32_t name##_lookahead_buffer[(lookahead_sz) / sizeof(uint32_t)];           \
    static struct lfs_config name = {                                                     \
        .read_size = (read_sz),                                                           \
        .prog_size = (prog_sz),                                                           \
        .cache_size = (cache_sz),                                                         \
        .lookahead_size = (lookahead_sz),                                                 \
        .read_buffer = name##_read_buffer,                                                \
        .prog_buffer = name##_prog_buffer,                                                \
        .lookahead_buffer = name##_lookahead_buffer,                                      \
    }

#define FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(name)                                                        \
    FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(name, CONFIG_FS_LITTLEFS_READ_SIZE, CONFIG_FS_LITTLEFS_PROG_SIZE, \
                                      CONFIG_FS_LITTLEFS_CACHE_SIZE, CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE)

int littlefs_init_cfg(struct lfs_config *cfg, uint32_t id);
int littlefs_read_directory(lfs_t *lfs, const char *dir);
int littlefs_mount(struct fs_mount_t *mountp);
int littlefs_unmount(struct fs_mount_t *mountp);
int32_t littlefs_write(struct fs_file_t *fp, const void *ptr, size_t len);
int littlefs_seek(struct fs_file_t *fp, int32_t off, int whence);
int32_t littlefs_read(struct fs_file_t *fp, void *ptr, size_t len);
int littlefs_open(struct fs_file_t *fp, const char *path, fs_mode_t flags);
int littlefs_close(struct fs_file_t *fp);
off_t littlefs_tell(struct fs_file_t *fp);
int littlefs_truncate(struct fs_file_t *fp, off_t length);
int littlefs_sync(struct fs_file_t *fp);
int littlefs_mkdir(struct fs_mount_t *mountp, const char *path);
int littlefs_opendir(struct fs_dir_t *dp, const char *path);
int littlefs_readdir(struct fs_dir_t *dp, struct fs_dirent *entry);
int littlefs_closedir(struct fs_dir_t *dp);
int littlefs_init(void);
int littlefs_rename(struct fs_mount_t *mountp, const char *from, const char *to);
int littlefs_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry);
int littlefs_statvfs(struct fs_mount_t *mountp, const char *path, struct fs_statvfs *stat);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LITTLEFS_FS_H*/
