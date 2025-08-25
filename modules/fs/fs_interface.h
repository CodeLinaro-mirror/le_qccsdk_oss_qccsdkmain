/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_
#define ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_NAME 250

/* Type for fs_open flags */
typedef uint8_t fs_mode_t;

struct fs_mount_t;

/**
 * @addtogroup file_system_api
 * @{
 */

/**
 * @brief File object representing an open file
 *
 * The object needs to be initialized with function fs_file_t_init().
 *
 * @param Pointer to FATFS file object structure
 * @param mp Pointer to mount point structure
 */
struct fs_file_t {
    void *filep;
    const struct fs_mount_t *mp;
    fs_mode_t flags;
};

/**
 * @brief Directory object representing an open directory
 *
 * The object needs to be initialized with function fs_dir_t_init().
 *
 * @param dirp Pointer to directory object structure
 * @param mp Pointer to mount point structure
 */
struct fs_dir_t {
    void *dirp;
    const struct fs_mount_t *mp;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FS_FS_INTERFACE_H_ */
