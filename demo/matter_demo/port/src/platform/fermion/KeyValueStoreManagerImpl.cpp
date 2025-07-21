/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Platform-specific key value storage implementation for QCA
 */

#include <platform/KeyValueStoreManager.h>
extern "C" {
#include "fcntl.h"
#include "fs.h"
#include <unistd.h>
#include "safeAPI.h"
}

namespace chip {
    namespace DeviceLayer {
        namespace Internal {
            extern char g_matter_kvs_dir[];
        }

        namespace PersistedStorage {

            /** Singleton instance of the KeyValueStoreManager implementation object.
             */
            KeyValueStoreManagerImpl KeyValueStoreManagerImpl::sInstance;

            CHIP_ERROR KeyValueStoreManagerImpl::_Get(const char *key, void *value, size_t value_size,
                                                      size_t *read_bytes_size, size_t offset)
            {
                VerifyOrReturnError(value, CHIP_ERROR_INVALID_ARGUMENT);
                VerifyOrReturnError(offset == 0, CHIP_ERROR_NOT_IMPLEMENTED);

                CHIP_ERROR err = CHIP_NO_ERROR;
                int status = 0, file = -1;
                struct fs_dirent entry;
                char *path = 0;
                uint8_t mount_len = strlen(chip::DeviceLayer::Internal::g_matter_kvs_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'
                uint32_t bytes_read;

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, chip::DeviceLayer::Internal::g_matter_kvs_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                for (int i = mount_len + 1; i < path_len; i++) {
                    if (path[i] == '/')
                        path[i] = '_';
                }

                status = vfs_stat(path, &entry);
                if (status == -ENOENT) {
                    err = CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
                    goto exit;
                }
                VerifyOrExit(status == 0, err = CHIP_ERROR_INTERNAL);
                VerifyOrExit(entry.size <= value_size, err = CHIP_ERROR_INTERNAL);

                file = open(path, O_RDONLY, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                bytes_read = read(file, value, entry.size);
                VerifyOrExit(bytes_read == entry.size, err = CHIP_ERROR_INTERNAL);

                if (read_bytes_size)
                    *read_bytes_size = bytes_read;

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR KeyValueStoreManagerImpl::_Put(const char *key, const void *value, size_t value_size)
            {
                VerifyOrReturnError(value, CHIP_ERROR_INVALID_ARGUMENT);

                CHIP_ERROR err = CHIP_NO_ERROR;
                int file = -1;
                char *path = 0;
                uint8_t mount_len = strlen(chip::DeviceLayer::Internal::g_matter_kvs_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'

                if (value_size == 0) {
                    return err;
                }

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, chip::DeviceLayer::Internal::g_matter_kvs_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                for (int i = mount_len + 1; i < path_len; i++) {
                    if (path[i] == '/')
                        path[i] = '_';
                }

                file = open(path, O_RDWR | O_CREAT, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                write(file, value, value_size);

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR KeyValueStoreManagerImpl::_Delete(const char *key)
            {
                int status = 0;
                struct fs_dirent entry;
                char *path = 0;
                uint8_t mount_len = strlen(chip::DeviceLayer::Internal::g_matter_kvs_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, chip::DeviceLayer::Internal::g_matter_kvs_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                for (int i = mount_len + 1; i < path_len; i++) {
                    if (path[i] == '/')
                        path[i] = '_';
                }

                status = vfs_stat(path, &entry);
                if (status == -ENOENT) {
                    free(path);
                    return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
                }

                status = unlink(path);
                free(path);
                VerifyOrReturnError(status == 0, CHIP_ERROR_INTERNAL);
                return CHIP_NO_ERROR;
            }

        }  // namespace PersistedStorage
    }      // namespace DeviceLayer
}  // namespace chip
