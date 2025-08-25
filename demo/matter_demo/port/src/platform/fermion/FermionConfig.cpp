/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Utilities for interacting with the the Fermion "NVS" key-value store.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/fermion/FermionConfig.h>

extern "C" {
#include "fcntl.h"
#include "fs.h"
#include <unistd.h>
}

namespace chip {
    namespace DeviceLayer {
        namespace Internal {

            extern const char g_matter_kvs_dir[];
            const char g_matter_dir[] = "/lfs/matter";
            const char g_matter_config_dir[] = "/lfs/matter/config";
            const char g_matter_kvs_dir[] = "/lfs/matter/kvs";

            FermionConfig::Key FermionConfig::kConfigKey_SetupDiscriminator = "discriminator";
            FermionConfig::Key FermionConfig::kConfigKey_SerialNum = "serialNum";
            FermionConfig::Key FermionConfig::kConfigKey_UniqueId = "uniqueID";
            FermionConfig::Key FermionConfig::kConfigKey_MfrDeviceId = "mfrDeviceId";
            FermionConfig::Key FermionConfig::kConfigKey_MfrDeviceCert = "mfrDeviceCert";
            FermionConfig::Key FermionConfig::kConfigKey_MfrDeviceICACerts = "mfrDeviceICACerts";
            FermionConfig::Key FermionConfig::kConfigKey_MfrDevicePrivateKey = "mfrDevicePrivateKey";
            FermionConfig::Key FermionConfig::kConfigKey_HardwareVersion = "hardwareVersion";
            FermionConfig::Key FermionConfig::kConfigKey_ManufacturingDate = "manufacturingDate";
            FermionConfig::Key FermionConfig::kConfigKey_SetupPinCode = "setupPinCode";
            FermionConfig::Key FermionConfig::kConfigKey_ServiceConfig = "serviceConfig";
            FermionConfig::Key FermionConfig::kConfigKey_PairedAccountId = "pairedAccountId";
            FermionConfig::Key FermionConfig::kConfigKey_ServiceId = "serviceId";
            FermionConfig::Key FermionConfig::kConfigKey_LastUsedEpochKeyId = "lastUsedEpochKeyId";
            FermionConfig::Key FermionConfig::kConfigKey_FailSafeArmed = "failSafeArmed";
            FermionConfig::Key FermionConfig::kConfigKey_WiFiStationSecType = "wifiStationSecType";
            FermionConfig::Key FermionConfig::kConfigKey_RegulatoryLocation = "regulatoryLocation";
            FermionConfig::Key FermionConfig::kConfigKey_CountryCode = "countryCode";
            FermionConfig::Key FermionConfig::kConfigKey_Spake2pIterationCount = "spake2pIterationCount";
            FermionConfig::Key FermionConfig::kConfigKey_Spake2pSalt = "spake2pSalt";
            FermionConfig::Key FermionConfig::kConfigKey_Spake2pVerifier = "spake2pVerifier";
            FermionConfig::Key FermionConfig::kConfigKey_DACCert = "DACCert";
            FermionConfig::Key FermionConfig::kConfigKey_DACPrivateKey = "DACPrivateKey";
            FermionConfig::Key FermionConfig::kConfigKey_DACPublicKey = "DACPublicKey";
            FermionConfig::Key FermionConfig::kConfigKey_PAICert = "PAICert";
            FermionConfig::Key FermionConfig::kConfigKey_CertDeclaration = "certDeclaration";
            FermionConfig::Key FermionConfig::kConfigKey_VendorId = "vendor-id";
            FermionConfig::Key FermionConfig::kConfigKey_VendorName = "vendor-name";
            FermionConfig::Key FermionConfig::kConfigKey_ProductId = "product-id";
            FermionConfig::Key FermionConfig::kConfigKey_ProductName = "product-name";

            FermionConfig::Key FermionConfig::kCounterKey_RebootCount = "rebootCount";
            FermionConfig::Key FermionConfig::kCounterKey_UpTime = "UpTime";
            FermionConfig::Key FermionConfig::kCounterKey_TotalOperationalHours = "totalOperationHours";

            CHIP_ERROR FermionConfig::Init()
            {
                int err = 0;
                struct fs_dir_t dir;

                fs_dir_t_init(&dir);
                err = vfs_opendir(&dir, g_matter_dir);
                /* Matter directory already exists */
                if (!err) {
                    vfs_closedir(&dir);
                    return CHIP_NO_ERROR;
                }

                err = vfs_mkdir(g_matter_dir);
                if (err)
                    return CHIP_ERROR_INTERNAL;

                err = vfs_mkdir(g_matter_config_dir);
                if (err)
                    return CHIP_ERROR_INTERNAL;

                err = vfs_mkdir(g_matter_kvs_dir);
                if (err)
                    return CHIP_ERROR_INTERNAL;

                return CHIP_NO_ERROR;
            }

            CHIP_ERROR FermionConfig::ReadConfigValue(Key key, uint32_t &val)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                int status = 0, file = -1;
                struct fs_dirent entry;
                char *path = 0;
                uint8_t mount_len = strlen(g_matter_config_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'
                uint32_t bytes_read;

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, g_matter_config_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                status = vfs_stat(path, &entry);
                if (status == -ENOENT) {
                    err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
                    goto exit;
                }
                VerifyOrExit(status == 0, err = CHIP_ERROR_INTERNAL);
                VerifyOrExit(entry.size <= sizeof(uint32_t), err = CHIP_ERROR_INTERNAL);

                file = open(path, O_RDONLY, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                bytes_read = read(file, &val, sizeof(uint32_t));
                VerifyOrExit(bytes_read == entry.size, err = CHIP_ERROR_INTERNAL);

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR FermionConfig::ReadConfigValueStr(Key key, char *buf, size_t bufSize, size_t &outLen)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                int status = 0, file = -1;
                struct fs_dirent entry;
                char *path = 0;
                uint8_t mount_len = strlen(g_matter_config_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'
                uint32_t bytes_read;

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, g_matter_config_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                status = vfs_stat(path, &entry);
                if (status == -ENOENT) {
                    err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
                    goto exit;
                }
                VerifyOrExit(status == 0, err = CHIP_ERROR_INTERNAL);
                VerifyOrExit(entry.size < bufSize, err = CHIP_ERROR_INTERNAL);

                file = open(path, O_RDONLY, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                bytes_read = read(file, buf, bufSize);
                VerifyOrExit(bytes_read == entry.size, err = CHIP_ERROR_INTERNAL);

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR FermionConfig::ReadConfigValueBin(Key key, uint8_t *buf, size_t bufSize, size_t &outLen)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                int status = 0, file = -1;
                struct fs_dirent entry;
                char *path = 0;
                uint8_t mount_len = strlen(g_matter_config_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'
                uint32_t bytes_read;

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, g_matter_config_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                status = vfs_stat(path, &entry);
                if (status == -ENOENT) {
                    err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
                    goto exit;
                }
                VerifyOrExit(status == 0, err = CHIP_ERROR_INTERNAL);
                VerifyOrExit(entry.size <= bufSize, err = CHIP_ERROR_INTERNAL);

                file = open(path, O_RDONLY, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                bytes_read = read(file, buf, bufSize);
                VerifyOrExit(bytes_read == entry.size, err = CHIP_ERROR_INTERNAL);

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR FermionConfig::WriteConfigValue(Key key, uint32_t val)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                int file = -1;
                char *path = 0;
                uint8_t mount_len = strlen(g_matter_config_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, g_matter_config_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                file = open(path, O_RDWR | O_CREAT, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                write(file, &val, sizeof(uint32_t));

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR FermionConfig::WriteConfigValueBin(Key key, const uint8_t *data, size_t dataLen)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                int file = -1;
                char *path = 0;
                uint8_t mount_len = strlen(g_matter_config_dir);
                uint8_t path_len = mount_len + strlen(key) + 2;  // 2 for '/' and '\0'

                path = (char *)malloc(path_len);
                VerifyOrReturnError(path != 0, CHIP_ERROR_INTERNAL);
                memcpy(path, g_matter_config_dir, mount_len);
                path[mount_len] = '/';
                strlcpy(&path[mount_len + 1], key, strlen(key) + 1);

                file = open(path, O_RDWR | O_CREAT, 0);
                VerifyOrExit(file != -1, err = CHIP_ERROR_INTERNAL);

                write(file, data, dataLen);

            exit:
                if (path)
                    free(path);
                if (file > 0)
                    close(file);

                return err;
            }

            CHIP_ERROR FermionConfig::WriteConfigValueStr(Key key, const char *str)
            {
                return WriteConfigValueStr(key, str, strlen(str));
            }

            CHIP_ERROR FermionConfig::WriteConfigValueStr(Key key, const char *str, size_t strLen)
            {
                return WriteConfigValueBin(key, (uint8_t *)str, strLen);
            }

            CHIP_ERROR FermionConfig::FactoryResetConfig(void)
            {
                int rc;
                struct fs_dir_t dir;
                struct fs_dirent entry;
                fs_dir_t_init(&dir);
                char path[64];
                uint8_t dir_len;

                rc = vfs_opendir(&dir, g_matter_dir);
                if (rc)
                    return CHIP_NO_ERROR;
                vfs_closedir(&dir);

                rc = vfs_opendir(&dir, g_matter_config_dir);
                if (!rc) {
                    dir_len = strlen(g_matter_config_dir);
                    while (1) {
                        rc = vfs_readdir(&dir, &entry);
                        if ((rc < 0) || (entry.type != FS_DIR_ENTRY_FILE)) {
                            vfs_closedir(&dir);
                            return CHIP_ERROR_INTERNAL;
                        }
                        if (entry.name[0] == 0) {
                            vfs_closedir(&dir);
                            unlink(g_matter_config_dir);
                            break;
                        }
                        memcpy(path, g_matter_config_dir, dir_len);
                        path[dir_len] = '/';
                        strlcpy(&path[dir_len + 1], entry.name, strlen(entry.name) + 1);
                        unlink(path);
                    }
                }

                rc = vfs_opendir(&dir, g_matter_kvs_dir);
                if (!rc) {
                    dir_len = strlen(g_matter_kvs_dir);
                    while (1) {
                        rc = vfs_readdir(&dir, &entry);
                        if ((rc < 0) || (entry.type != FS_DIR_ENTRY_FILE)) {
                            vfs_closedir(&dir);
                            return CHIP_ERROR_INTERNAL;
                        }
                        if (entry.name[0] == 0) {
                            vfs_closedir(&dir);
                            unlink(g_matter_kvs_dir);
                            break;
                        }
                        memcpy(path, g_matter_kvs_dir, dir_len);
                        path[dir_len] = '/';
                        strlcpy(&path[dir_len + 1], entry.name, strlen(entry.name) + 1);
                        unlink(path);
                    }
                }

                unlink(g_matter_dir);
                return CHIP_NO_ERROR;
            }

        }  // namespace Internal
    }      // namespace DeviceLayer
}  // namespace chip
