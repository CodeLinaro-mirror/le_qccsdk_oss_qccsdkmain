/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include "stdio.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "qapi_flash.h"
#include "qapi_firmware_upgrade.h"
#include "lfs.h"
#include "semphr.h"
#include "littlefs_fs.h"
#include "fs.h"
#include "fs_sys.h"
#include <stdio.h>
#include "autoconf.h"

#undef DEBUG_LFS
#define DEBUG_LFS

#ifdef DEBUG_LFS
#define DEBUG_LFS_LOG(args...) printf(args)
#define LOG_INF(args...)       printf(args)
#define LOG_ERR(args...)       printf(args)
#else
#define DEBUG_LFS_LOG(args...)
#define LOG_INF(args...)
#define LOG_ERR(args...)
#endif

struct lfs_file_data {
    struct lfs_file file;
    volatile char used;
};

struct lfs_file_data file_pool[CONFIG_FS_LITTLEFS_NUM_FILES];

static void alloc_lfs_file_data(struct lfs_file_data **fdpp)
{
    int i;
    for (i = 0; i < CONFIG_FS_LITTLEFS_NUM_FILES; i++) {
        if (file_pool[i].used == 0) {
            *fdpp = &file_pool[i];
            (*fdpp)->used = 1;
            return;
        }
    }
    *fdpp = NULL;
}

static void free_lfs_file_data(struct lfs_file_data **fdpp)
{
    memset(*fdpp, 0, sizeof(struct lfs_file_data));
    *fdpp = NULL;
}

struct lfs_dir_data {
    struct lfs_dir dir;
    volatile char used;
};

struct lfs_dir_data dir_pool[CONFIG_FS_LITTLEFS_NUM_DIRS];

static void alloc_lfs_dir_data(struct lfs_dir_data **ddpp)
{
    int i;
    for (i = 0; i < CONFIG_FS_LITTLEFS_NUM_DIRS; i++) {
        if (dir_pool[i].used == 0) {
            // printf("dir at %d\r\n",i);
            *ddpp = &dir_pool[i];
            dir_pool[i].used = 1;
            return;
        }
    }
    *ddpp = NULL;
}

static void free_lfs_dir_data(struct lfs_dir_data **ddpp)
{
    memset(*ddpp, 0, sizeof(struct lfs_dir_data));
    *ddpp = NULL;
}

static int lfs_flags(unsigned int zflags)
{
    int flags = (zflags & FS_O_CREATE) ? LFS_O_CREAT : 0;
    flags |= (zflags & FS_O_READ) ? LFS_O_RDONLY : 0;
    flags |= (zflags & FS_O_WRITE) ? LFS_O_WRONLY : 0;
    flags |= (zflags & FS_O_APPEND) ? LFS_O_APPEND : 0;
    return flags;
}

const char *strip_prefix(const char *path, const struct fs_mount_t *mp)
{
    static const char *const root = "/";
    if ((path == NULL) || (mp == NULL))
        return path;
    path += mp->mountp_len;
    return *path ? path : root;
}

#define LFS_FILEP(fp) (&((struct lfs_file_data *)(fp->filep))->file)

static inline void fs_lock(struct fs_littlefs *fs)
{
    xSemaphoreTake(fs->mutex, portMAX_DELAY);
}

static inline void fs_unlock(struct fs_littlefs *fs)
{
    xSemaphoreGive(fs->mutex);
}

static int lfs_to_errno(int error)
{
    if (error >= 0) {
        return error;
    }

    switch (error) {
        default:
        case LFS_ERR_IO: /* Error during device operation */
            return -EIO;
        case LFS_ERR_CORRUPT: /* Corrupted */
            return -EFAULT;
        case LFS_ERR_NOENT: /* No directory entry */
            return -ENOENT;
        case LFS_ERR_EXIST: /* Entry already exists */
            return -EEXIST;
        case LFS_ERR_NOTDIR: /* Entry is not a dir */
            return -ENOTDIR;
        case LFS_ERR_ISDIR: /* Entry is a dir */
            return -EISDIR;
        case LFS_ERR_NOTEMPTY: /* Dir is not empty */
            return -ENOTEMPTY;
        case LFS_ERR_BADF: /* Bad file number */
            return -EBADF;
        case LFS_ERR_FBIG: /* File too large */
            return -EFBIG;
        case LFS_ERR_INVAL: /* Invalid parameter */
            return -EINVAL;
        case LFS_ERR_NOSPC: /* No space left on device */
            return -ENOSPC;
        case LFS_ERR_NOMEM: /* No more memory available */
            return -ENOMEM;
    }
}

static int get_fs_partition_info(uint32_t id, uint32_t *fs_start_addr, uint32_t *fs_size)
{
    uint8_t Index;
    uint32_t boot_type, fwd_present;
    int32_t ret;
    qapi_Part_Hdl_t hdl;

    ret = qapi_Fw_Upgrade_init();
    if (ret != QAPI_OK) {
        goto out;
    }

    Index = qapi_Fw_Upgrade_Get_Active_FWD(&boot_type, &fwd_present);

    // id = FS1_IMG_ID  or FS2_IMG_ID
    ret = qapi_Fw_Upgrade_Find_Partition(Index, id, &hdl);
    if (ret != QAPI_OK) {
        goto out;
    }
    ret = qapi_Fw_Upgrade_Get_Partition_Start(hdl, fs_start_addr);
    if (ret != QAPI_OK) {
        goto out;
    }
    ret = qapi_Fw_Upgrade_Get_Partition_Size(hdl, fs_size);
    if (ret != QAPI_OK) {
        goto out;
    }

    DEBUG_LFS_LOG("FS%d start:0x%x, size:0x%x\n", id, *fs_start_addr, *fs_size);  // will only use FS1 to write/read
out:
    DEBUG_LFS_LOG("%s:%d\r\n", __func__, ret);
    return ret;
}

static int lfs_api_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    int status =
        qapi_Flash_Read((uint32_t)((uint32_t)(c->context) + block * c->block_size + off), size, (uint8_t *)buffer);
    // DEBUG_LFS_LOG("%s %d\r\n",__func__, status);
    return status;
}

static int lfs_api_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer,
                        lfs_size_t size)
{
    int ret =
        qapi_Flash_Write((uint32_t)((uint32_t)(c->context) + block * c->block_size + off), size, (uint8_t *)buffer);
    // DEBUG_LFS_LOG("%s %d\r\n",__func__, ret);
    return ret;
}

static int lfs_api_erase(const struct lfs_config *c, lfs_block_t block)
{
    // DEBUG_LFS_LOG("%s start_addr:0x%x size:%d\r\n", __func__,(uint32_t)((uint32_t)(c->context) + block *
    // c->block_size)/0x1000, c->block_size/0x1000);
    int ret =
        qapi_Flash_Erase(QAPI_FLASH_BLOCK_ERASE_E, (uint32_t)((uint32_t)(c->context) + block * c->block_size) / 0x1000,
                         c->block_size / 0x1000);
    // DEBUG_LFS_LOG("%s %d\r\n",__func__, ret);
    return ret;
}

static int lfs_api_sync(const struct lfs_config *c)
{
    (void)c;
    // DEBUG_LFS_LOG("%s \r\n",__func__);
    return LFS_ERR_OK;
}

int littlefs_init_cfg(struct lfs_config *cfg, uint32_t id)
{
    uint32_t fs_size = 0;
    uint32_t fs_start_addr = 0;

    uint32_t ret = 0;
    ret = get_fs_partition_info(id, &fs_start_addr, &fs_size);
#ifdef CONFIG_MATTER_ENABLE
    fs_size = 0x20000;  // enlarge fs_size for Matter demo
#endif
    if (ret != QAPI_OK)
        goto out;

    cfg->context = (void *)fs_start_addr;
    cfg->read = lfs_api_read;
    cfg->prog = lfs_api_prog;
    cfg->erase = lfs_api_erase;
    cfg->sync = lfs_api_sync;

    cfg->block_size = CONFIG_FS_LITTLEFS_BLOCK_SIZE;             // 4k
    cfg->block_count = fs_size / CONFIG_FS_LITTLEFS_BLOCK_SIZE;  // 16
    cfg->block_cycles = CONFIG_FS_LITTLEFS_BLOCK_CYCLES;         // 512

    // cfg->read_size = CONFIG_FS_LITTLEFS_READ_SIZE; // 16
    // cfg->prog_size = CONFIG_FS_LITTLEFS_PROG_SIZE ; // 16
    // cfg->cache_size = CONFIG_FS_LITTLEFS_CACHE_SIZE; //64
    // cfg->lookahead_size = CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE; //32

    // DEBUG_LFS_LOG("read_size:%d, prog_size:%d, block_size:%d, block_count:%d, cache_size:%d, lookahead_size:%d,
    // block_cycles:%d\r\n", cfg->read_size, cfg->prog_size, cfg->block_size, cfg->block_count, cfg->cache_size,
    // cfg->lookahead_size, cfg->block_cycles);
out:
    return ret;
}

int littlefs_read_directory(lfs_t *lfs, const char *dir)
{
    // DEBUG_LFS_LOG("in %s dir:%s\r\n", __func__, dir);
    lfs_dir_t dir_obj;
    struct lfs_info info;

    int err = lfs_dir_open(lfs, &dir_obj, dir);
    if (err) {
        DEBUG_LFS_LOG("Failed to open directory %s err:%d\n", dir, err);
        return err;
    }

    while ((err = lfs_dir_read(lfs, &dir_obj, &info)) > 0) {
        if (info.type == LFS_TYPE_REG) {
            DEBUG_LFS_LOG("Regular file: %s size:0x%x\n", info.name, info.size);
        } else if (info.type == LFS_TYPE_DIR) {
            DEBUG_LFS_LOG("Directory: %s size:0x%x\n", info.name, info.size);
            if ((strcmp((char *)info.name, ".") != 0) && (strcmp((char *)info.name, "..") != 0)) {
                DEBUG_LFS_LOG("read subdir %s\r\n", info.name);
                littlefs_read_directory(lfs, (const char *)info.name);
            }
        }
    }
    // printf("err is %d when break the while\r\n", err);
    if (err < LFS_ERR_OK) {
        DEBUG_LFS_LOG("Error reading directory %s: %d\n", dir, err);
    }
    lfs_dir_close(lfs, &dir_obj);
    DEBUG_LFS_LOG("out %s dir:%s\r\n", __func__, dir);
    return err;
}

extern struct fs_littlefs fs;

int littlefs_open(struct fs_file_t *fp, const char *path, fs_mode_t zflags)
{
    // printf("%s path:%s\r\n",__func__, path);
    struct fs_littlefs *fs = fp->mp->fs_data;
    int ret;
    int flags = lfs_flags(zflags);

    alloc_lfs_file_data((struct lfs_file_data **)(&fp->filep));
    if (fp->filep == NULL) {
        LOG_ERR("No memory to open more file!\r\n");
        return -ENOMEM;
    }

    struct lfs_file_data *fdp = fp->filep;
    path = strip_prefix(path, fp->mp);
    fs_lock(fs);
    ret = lfs_file_open(&fs->lfs, &fdp->file, path, flags);
    fs_unlock(fs);

    if (ret < 0) {
        free_lfs_file_data(&fdp);
        LOG_ERR("open file:%s failed:%d\r\n", path, ret);
    }

    return ret;
}

int littlefs_close(struct fs_file_t *fp)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_close(&fs->lfs, LFS_FILEP(fp));
    fs_unlock(fs);
    struct lfs_file_data *fdp = fp->filep;
    free_lfs_file_data(&fdp);
    return ret;
}

int32_t littlefs_read(struct fs_file_t *fp, void *ptr, size_t len)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_read(&fs->lfs, LFS_FILEP(fp), ptr, len);
    fs_unlock(fs);
    return ret;
}

int littlefs_seek(struct fs_file_t *fp, int32_t off, int whence)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_seek(&fs->lfs, LFS_FILEP(fp), off, whence);
    fs_unlock(fs);
    return ret;
}

int32_t littlefs_write(struct fs_file_t *fp, const void *ptr, size_t len)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_write(&fs->lfs, LFS_FILEP(fp), ptr, len);
    fs_unlock(fs);
    return ret;
}

int littlefs_rename(struct fs_mount_t *mountp, const char *from, const char *to)
{
    struct fs_littlefs *fs = mountp->fs_data;

    from = strip_prefix(from, mountp);
    to = strip_prefix(to, mountp);

    fs_lock(fs);

    int ret = lfs_rename(&fs->lfs, from, to);

    fs_unlock(fs);
    return (ret);
}

int littlefs_unlink(struct fs_mount_t *mountp, const char *path)
{
    struct fs_littlefs *fs = mountp->fs_data;

    path = strip_prefix(path, mountp);

    fs_lock(fs);

    int ret = lfs_remove(&fs->lfs, path);

    fs_unlock(fs);

    return lfs_to_errno(ret);
}

static void info_to_dirent(const struct lfs_info *info, struct fs_dirent *entry)
{
    entry->type = ((info->type == LFS_TYPE_DIR) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE);
    entry->size = info->size;
    strlcpy(entry->name, info->name, sizeof(entry->name));
    entry->name[sizeof(entry->name) - 1] = '\0';
}

int littlefs_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry)
{
    struct fs_littlefs *fs = mountp->fs_data;

    path = strip_prefix(path, mountp);

    fs_lock(fs);

    struct lfs_info info;
    int ret = lfs_stat(&fs->lfs, path, &info);

    fs_unlock(fs);

    if (ret >= 0) {
        info_to_dirent(&info, entry);
        ret = 0;
    }

    return (ret);
}

int littlefs_statvfs(struct fs_mount_t *mountp, const char *path, struct fs_statvfs *stat)
{
    struct fs_littlefs *fs = mountp->fs_data;
    struct lfs *lfs = &fs->lfs;

    stat->f_bsize = lfs->cfg->prog_size;
    stat->f_frsize = lfs->cfg->block_size;
    stat->f_blocks = lfs->cfg->block_count;

    path = strip_prefix(path, mountp);

    fs_lock(fs);

    ssize_t ret = lfs_fs_size(lfs);

    fs_unlock(fs);

    if (ret >= 0) {
        stat->f_bfree = stat->f_blocks - ret;
        ret = 0;
    }

    return (ret);
}

off_t littlefs_tell(struct fs_file_t *fp)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_tell(&fs->lfs, LFS_FILEP(fp));
    fs_unlock(fs);
    return ret;
}

int littlefs_truncate(struct fs_file_t *fp, off_t length)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_truncate(&fs->lfs, LFS_FILEP(fp), length);
    fs_unlock(fs);
    return ret;
}

int littlefs_sync(struct fs_file_t *fp)
{
    struct fs_littlefs *fs = fp->mp->fs_data;
    fs_lock(fs);
    int ret = lfs_file_sync(&fs->lfs, LFS_FILEP(fp));
    fs_unlock(fs);
    return ret;
}

int littlefs_mkdir(struct fs_mount_t *mountp, const char *path)
{
    struct fs_littlefs *fs = mountp->fs_data;
    path = strip_prefix(path, mountp);
    fs_lock(fs);
    int ret = lfs_mkdir(&fs->lfs, path);
    fs_unlock(fs);
    return ret;
}

int littlefs_opendir(struct fs_dir_t *dp, const char *path)
{
    struct fs_littlefs *fs = dp->mp->fs_data;

    alloc_lfs_dir_data((struct lfs_dir_data **)(&dp->dirp));
    if (dp->dirp == NULL) {
        LOG_ERR("No memory to open more dir!\r\n");
        return -ENOMEM;
    }

    struct lfs_dir_data *ddp = dp->dirp;
    path = strip_prefix(path, dp->mp);
    // printf("[littlefs_opendir]now to open %s\r\n", path);
    fs_lock(fs);
    // printf("[littlefs_opendir]!!!!!!!!dir at %p\r\n", &ddp->dir);
    int ret = lfs_dir_open(&fs->lfs, &ddp->dir, path);
    fs_unlock(fs);

    if (ret < 0) {
        free_lfs_dir_data(&ddp);
    }

    return ret;
}

int littlefs_readdir(struct fs_dir_t *dp, struct fs_dirent *entry)
{
    struct fs_littlefs *fs = dp->mp->fs_data;
    struct lfs_dir_data *ddp = dp->dirp;

    fs_lock(fs);
    struct lfs_info info;
    int ret = lfs_dir_read(&fs->lfs, &ddp->dir, &info);
    fs_unlock(fs);

    if (ret > 0) {
        info_to_dirent(&info, entry);
        ret = 0;
    } else if (ret == 0) {
        entry->name[0] = 0;
    }

    return (ret);
}

int littlefs_closedir(struct fs_dir_t *dp)
{
    struct fs_littlefs *fs = dp->mp->fs_data;
    struct lfs_dir_data *ddp = dp->dirp;

    fs_lock(fs);
    int ret = lfs_dir_close(&fs->lfs, &ddp->dir);
    fs_unlock(fs);

    free_lfs_dir_data(&ddp);

    return (ret);
}

int littlefs_mount(struct fs_mount_t *mountp)
{
    int ret = 0;
    struct fs_littlefs *fs = mountp->fs_data;

    fs->mutex = xSemaphoreCreateMutex();
    fs_lock(fs);

    ret = littlefs_init_cfg(fs->cfg, fs->id);
    if (ret != 0)
        goto out;

    ret = lfs_mount(&fs->lfs, fs->cfg);

    if (ret < 0) {
        ret = lfs_format(&fs->lfs, fs->cfg);
        if (ret < 0) {
            DEBUG_LFS_LOG("lfs_format error:%d\r\n", ret);
            goto out;
        }

        ret = lfs_mount(&fs->lfs, fs->cfg);
        if (ret < 0) {
            DEBUG_LFS_LOG("remount failed after format:%d\r\n", ret);
            goto out;
        }
    }

    DEBUG_LFS_LOG("LFS mounted\r\n");

out:
    DEBUG_LFS_LOG("%s:%d\r\n", __func__, ret);
    fs_unlock(fs);
    return ret;
}

int littlefs_unmount(struct fs_mount_t *mountp)
{
    struct fs_littlefs *fs = mountp->fs_data;
    fs_lock(fs);
    lfs_unmount(&fs->lfs);
    fs_unlock(fs);
    LOG_INF("%s unmounted\r\n", mountp->mnt_point);
    return 0;
}

static const struct fs_file_system_t littlefs_fs = {
    .open = littlefs_open,
    .close = littlefs_close,
    .read = littlefs_read,
    .write = littlefs_write,
    .lseek = littlefs_seek,
    .tell = littlefs_tell,
    .truncate = littlefs_truncate,
    .sync = littlefs_sync,
    .opendir = littlefs_opendir,
    .readdir = littlefs_readdir,
    .closedir = littlefs_closedir,
    .mkdir = littlefs_mkdir,
    .rename = littlefs_rename,
    .stat = littlefs_stat,
    .unlink = littlefs_unlink,
    .statvfs = littlefs_statvfs,
    .unmount = littlefs_unmount,
    .mount = littlefs_mount,
};

int littlefs_init(void)
{
    int rc = vfs_register(FS_LITTLEFS, &littlefs_fs);
    return rc;
}
