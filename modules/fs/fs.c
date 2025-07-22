/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "fs_sys.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "list.h"
#include "littlefs_fs.h"
#include "fw_upgrade_mem.h"

#undef DEBUG_FS

#ifdef DEBUG_FS
#define DEBUG_FS_LOG(args...) printf(args)
#define LOG_DBG(args...)      printf(args)
#define LOG_ERR(args...)      printf(args)
#else
#define DEBUG_FS_LOG(args...)
#define LOG_DBG(args...)
#define LOG_ERR(args...)
#endif

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cfg);

struct fs_littlefs fs = {
    .id = FS1_IMG_ID,
    .cfg = &cfg,
};

static struct fs_mount_t lfs_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &fs,
    .mnt_point = "/lfs",
};

struct fs_mount_t *mp = &lfs_mnt;

static List_t fs_mnt_list;

static int g_fs_mounted;

SemaphoreHandle_t mutex;

struct registry_entry {
    int type;
    const struct fs_file_system_t *fstp;
};

static struct registry_entry registry[CONFIG_FILE_SYSTEM_MAX_TYPES];

static inline void registry_clear_entry(struct registry_entry *ep)
{
    ep->fstp = NULL;
}

static int registry_add(int type, const struct fs_file_system_t *fstp)
{
    int ret = -ENOSPC;
    int i;
    for (i = 0; i < CONFIG_FILE_SYSTEM_MAX_TYPES; i++) {
        struct registry_entry *ep = &registry[i];
        if (ep->fstp == NULL) {
            ep->type = type;
            ep->fstp = fstp;
            ret = 0;
            break;
        }
    }

    return ret;
}

static struct registry_entry *registry_find(int type)
{
    int i;
    for (i = 0; i < CONFIG_FILE_SYSTEM_MAX_TYPES; i++) {
        struct registry_entry *ep = &registry[i];
        if ((ep->fstp != NULL) && (ep->type == type))
            return ep;
    }
    return NULL;
}

static const struct fs_file_system_t *fs_type_get(int type)
{
    struct registry_entry *ep = registry_find(type);
    return (ep != NULL) ? ep->fstp : NULL;
}

static int fs_get_mnt_point(struct fs_mount_t **mnt_pntp, const char *name, size_t *match_len)
{
    struct fs_mount_t *mnt_p = NULL, *itr;
    size_t longest_match = 0;

    ListItem_t *pxListItem;
    ListItem_t const *pxListEnd;

    size_t len, name_len = strlen(name);

    xSemaphoreTake(mutex, portMAX_DELAY);
    int32_t num = listCURRENT_LIST_LENGTH(&fs_mnt_list);
    // LOG_DBG("num is %d\r\n", num);

    if (!(listLIST_IS_EMPTY(&fs_mnt_list))) {
        pxListItem = listGET_HEAD_ENTRY(&fs_mnt_list);
        pxListEnd = listGET_END_MARKER(&fs_mnt_list);

        for (; pxListItem != pxListEnd; pxListItem = listGET_NEXT(pxListItem)) {
            itr = (struct fs_mount_t *)listGET_LIST_ITEM_OWNER(pxListItem);
            if (itr == NULL) {
                LOG_ERR("mp is NULL\r\n");
                // continue;
                break;
            }
            len = itr->mountp_len;
            if ((len < longest_match) || (len > name_len))
                continue;

            if ((len > 1) && (name[len] != '/') && (name[len] != '\0'))
                continue;

            if (strncmp(name, itr->mnt_point, len) == 0) {
                mnt_p = itr;
                longest_match = len;
                // printf("%s, fs:%p, mnt_p:%p\r\n",__func__, itr->fs, mnt_p);
                // LOG_DBG("match: len:%d, prefix:%s\r\n", len, itr->mnt_point);
            }
        }
    }
    xSemaphoreGive(mutex);
    if (mnt_p == NULL)
        return -ENOENT;

    *mnt_pntp = mnt_p;
    // printf("%s *mnt_pntp:%p\r\n", __func__, *mnt_pntp);
    if (match_len)
        *match_len = mnt_p->mountp_len;

    return 0;
}

int vfs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
    // printf("in vfs_open\n");
    struct fs_mount_t *mp;
    int rc = -EINVAL;

    if ((file_name == NULL) || (strlen(file_name) <= 1) || (file_name[0] != '/')) {
        LOG_ERR("invalid file name!!\r\n");
        return rc;
    }

    if (zfp->mp != NULL) {
        return -EBUSY;
    }

    rc = fs_get_mnt_point(&mp, file_name, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    // printf("%s mp:%p mp->fs:%p open:%p\r\n", __func__, mp, mp->fs, mp->fs->open);
    if (mp->fs->open == NULL) {
        // printf("fs not support open\r\n");
        return -ENOTSUP;
    }

    zfp->mp = mp;
    rc = mp->fs->open(zfp, file_name, flags);
    if (rc < 0) {
        LOG_ERR("file open error (%d)\r\n", rc);
        zfp->mp = NULL;
        return rc;
    }

    zfp->flags = flags;
    return rc;
}

int vfs_close(struct fs_file_t *zfp)
{
    int rc = -EINVAL;

    if (zfp->mp == NULL) {
        LOG_DBG("not opened!\r\n");
        return 0;
    }

    if (zfp->mp->fs->close == NULL)
        return -ENOTSUP;

    rc = zfp->mp->fs->close(zfp);
    if (rc < 0) {
        LOG_ERR("file close error (%d)\r\n", rc);
        return rc;
    }

    zfp->mp = NULL;
    return rc;
}

int32_t vfs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
    int rc = -EINVAL;

    if (zfp->mp == NULL)
        return -EBADF;

    if (zfp->mp->fs->read == NULL)
        return -ENOTSUP;

    rc = zfp->mp->fs->read(zfp, ptr, size);
    if (rc < 0) {
        LOG_ERR("file read error (%d)\r\n", rc);
    }
    return rc;
}

int32_t vfs_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
    int rc = -EINVAL;
    if (zfp->mp == NULL)
        return -EBADF;

    if (zfp->mp->fs->write == NULL)
        return -ENOTSUP;

    rc = zfp->mp->fs->write(zfp, ptr, size);
    if (rc < 0) {
        LOG_ERR("file write error (%d)\r\n", rc);
    }

    return rc;
}

int vfs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
    int rc = -ENOTSUP;

    if (zfp->mp == NULL)
        return -EBADF;
    if (zfp->mp->fs->lseek == NULL)
        return -ENOTSUP;
    rc = zfp->mp->fs->lseek(zfp, offset, whence);
    if (rc < 0) {
        LOG_ERR("file seek error (%d)", rc);
    }
    return rc;
}

off_t vfs_tell(struct fs_file_t *zfp)
{
    int rc = -ENOTSUP;

    if (zfp->mp == NULL)
        return -EBADF;

    if (zfp->mp->fs->tell == NULL)
        return -ENOTSUP;

    rc = zfp->mp->fs->tell(zfp);
    if (rc < 0) {
        LOG_ERR("file tell error (%d)\r\n", rc);
    }

    return rc;
}

int vfs_truncate(struct fs_file_t *zfp, off_t length)
{
    int rc = -EINVAL;

    if (zfp->mp == NULL) {
        return -EBADF;
    }

    if (zfp->mp->fs->truncate == NULL) {
        return -ENOTSUP;
    }

    rc = zfp->mp->fs->truncate(zfp, length);
    if (rc < 0) {
        LOG_ERR("file truncate error (%d)\r\n", rc);
    }

    return rc;
}

int vfs_sync(struct fs_file_t *zfp)
{
    int rc = -EINVAL;

    if (zfp->mp == NULL) {
        return -EBADF;
    }

    if (zfp->mp->fs->sync == NULL) {
        return -ENOTSUP;
    }

    rc = zfp->mp->fs->sync(zfp);
    if (rc < 0) {
        LOG_ERR("file sync error (%d)\r\n", rc);
    }

    return rc;
}

int vfs_rename(const char *from, const char *to)
{
    struct fs_mount_t *mp;
    size_t match_len;
    int rc = -EINVAL;

    if ((from == NULL) || (strlen(from) <= 1) || (from[0] != '/') || (to == NULL) || (strlen(to) <= 1) ||
        (to[0] != '/')) {
        LOG_ERR("invalid file name!!\r\n");
        return -EINVAL;
    }

    rc = fs_get_mnt_point(&mp, from, &match_len);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    if (mp->flags & FS_MOUNT_FLAG_READ_ONLY) {
        return -EROFS;
    }

    /* Make sure both files are mounted on the same path */
    if (strncmp(from, to, match_len) != 0) {
        LOG_ERR("mount point not same!!\r\n");
        return -EINVAL;
    }

    if (mp->fs->rename == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->rename(mp, from, to);
    if (rc < 0) {
        LOG_ERR("failed to rename file or dir (%d)\r\n", rc);
    }

    return rc;
}

int vfs_unlink(const char *abs_path)
{
    struct fs_mount_t *mp;
    int rc = -EINVAL;

    if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid file name!!");
        return -EINVAL;
    }

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!");
        return rc;
    }

    if (mp->flags & FS_MOUNT_FLAG_READ_ONLY) {
        return -EROFS;
    }

    if (mp->fs->unlink == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->unlink(mp, abs_path);
    if (rc < 0) {
        LOG_ERR("failed to unlink path (%d)", rc);
    }

    return rc;
}

int fs_stat(const char *abs_path, struct fs_dirent *entry)
{
    struct fs_mount_t *mp;
    int rc = -EINVAL;

    if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid file or dir name!!\r\n");
        return -EINVAL;
    }

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    if (mp->fs->stat == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->stat(mp, abs_path, entry);
    if (rc == -ENOENT) {
        /* File doesn't exist, which is a valid stat response */
    } else if (rc < 0) {
        LOG_ERR("failed get file or dir stat (%d)\r\n", rc);
    }
    return rc;
}

int vfs_stat(const char *abs_path, struct fs_dirent *entry)
{
    struct fs_mount_t *mp;
    int rc = -EINVAL;

    if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid file or dir name!!");
        return -EINVAL;
    }

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!");
        return rc;
    }

    if (mp->fs->stat == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->stat(mp, abs_path, entry);
    if (rc == -ENOENT) {
        /* File doesn't exist, which is a valid stat response */
    } else if (rc < 0) {
        LOG_ERR("failed get file or dir stat (%d)", rc);
    }
    return rc;
}

int vfs_statvfs(const char *abs_path, struct fs_statvfs *stat)
{
    struct fs_mount_t *mp;
    int rc;

    if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid file or dir name!!\r\n");
        return -EINVAL;
    }

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    if (mp->fs->statvfs == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->statvfs(mp, abs_path, stat);
    if (rc < 0) {
        LOG_ERR("failed get file or dir stat (%d)\r\n", rc);
    }

    return rc;
}

static int check_directory_depth(const char *abs_path)
{
    int depth = 0;
    size_t len = strlen(abs_path);

    while (len > 0 && abs_path[len - 1] == '/') {
        len--;
    }

    for (size_t i = 0; i < len; i++) {
        if (abs_path[i] == '/') {
            depth++;
        }
    }

    depth--;

    if (depth <= CONFIG_MAX_DIRECTORY_DEPTH)
        return 0;
    else
        return -1;
}

int vfs_mkdir(const char *abs_path)
{
    struct fs_mount_t *mp;
    int rc = -ENOTSUP;

    if ((abs_path == NULL) || (strlen(abs_path) <= 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid directory name!!\r\n");
        return -EINVAL;
    }

    if (check_directory_depth(abs_path))
        return rc;

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    if (mp->fs->mkdir == NULL) {
        return -ENOTSUP;
    }

    rc = mp->fs->mkdir(mp, abs_path);
    if (rc < 0) {
        if (rc == -EEXIST) {
            LOG_ERR("directory:%s exists.\r\n", abs_path);
        } else {
            LOG_ERR("failed to create directory:%s (%d)\r\n", abs_path, rc);
        }
    }

    return rc;
}

int vfs_opendir(struct fs_dir_t *zdp, const char *abs_path)
{
    struct fs_mount_t *mp;
    int rc = -EINVAL;

    if ((abs_path == NULL) || (strlen(abs_path) < 1) || (abs_path[0] != '/')) {
        LOG_ERR("invalid directory name!!\r\n");
        return -EINVAL;
    }

    if (zdp->mp != NULL || zdp->dirp != NULL) {
        return -EBUSY;
    }

    // printf("[vfs_opendir] dir:%s\r\n", abs_path);

#if 0
    if (strcmp(abs_path, "/") == 0)
    {
        /* Open VFS root dir, marked by zdp->mp == NULL */
        k_mutex_lock(&mutex, K_FOREVER);

        zdp->mp = NULL;
        zdp->dirp = sys_dlist_peek_head(&fs_mnt_list);

        k_mutex_unlock(&mutex);

        return 0;
    }
#endif

    rc = fs_get_mnt_point(&mp, abs_path, NULL);
    if (rc < 0) {
        LOG_ERR("mount point not found!!\r\n");
        return rc;
    }

    if (mp->fs->opendir == NULL) {
        return -ENOTSUP;
    }

    zdp->mp = mp;
    rc = zdp->mp->fs->opendir(zdp, abs_path);
    if (rc < 0) {
        zdp->mp = NULL;
        zdp->dirp = NULL;
        LOG_ERR("directory open error (%d)\r\n", rc);
    }

    return rc;
}

int vfs_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
    if (zdp->mp) {
        /* Delegate to mounted filesystem */
        int rc = -EINVAL;

        if (zdp->mp->fs->readdir == NULL) {
            return -ENOTSUP;
        }

        /* Loop until error or not special directory */
        while (1) {
            rc = zdp->mp->fs->readdir(zdp, entry);
            if (rc < 0) {
                break;
            }
            if (entry->name[0] == 0) {
                break;
            }
            if (entry->type != FS_DIR_ENTRY_DIR) {
                break;
            }
            if ((strcmp(entry->name, ".") != 0) && (strcmp(entry->name, "..") != 0)) {
                break;
            }
        }
        if (rc < 0) {
            LOG_ERR("directory read error (%d)\r\n", rc);
        }

        return rc;
    }

    return 0;
}

int vfs_closedir(struct fs_dir_t *zdp)
{
    int rc = -EINVAL;

    if (zdp->mp == NULL) {
        /* VFS root dir */
        zdp->dirp = NULL;
        return 0;
    }

    if (zdp->mp->fs->closedir == NULL) {
        return -ENOTSUP;
    }

    rc = zdp->mp->fs->closedir(zdp);
    if (rc < 0) {
        LOG_ERR("directory close error (%d)\r\n", rc);
        return rc;
    }

    zdp->mp = NULL;
    zdp->dirp = NULL;
    return rc;
}

int vfs_register(int type, const struct fs_file_system_t *fs)
{
    int ret = 0;

    // printf("%s, fs:%p\r\n",__func__, fs);

    xSemaphoreTake(mutex, portMAX_DELAY);

    if (fs_type_get(type) != NULL) {
        DEBUG_FS_LOG("already register:%d\r\n", type);
        ret = -1;
    } else {
        ret = registry_add(type, fs);
    }

    xSemaphoreGive(mutex);

    DEBUG_FS_LOG("fs register %d:%d\r\n", type, ret);
    return ret;
}

int vfs_unregister(int type, const struct fs_file_system_t *fs)
{
    int rc = 0;
    struct registry_entry *ep;

    xSemaphoreTake(mutex, portMAX_DELAY);

    ep = registry_find(type);
    if ((ep == NULL) || (ep->fstp != fs)) {
        rc = -EINVAL;
    } else {
        registry_clear_entry(ep);
    }

    xSemaphoreGive(mutex);

    DEBUG_FS_LOG("fs unregister %d: %d\r\n", type, rc);
    return rc;
}

int vfs_mount(struct fs_mount_t *mp)
{
    int rc = -EINVAL;
    size_t len = 0;
    struct fs_mount_t *itr;
    const struct fs_file_system_t *fs;

    if ((mp == NULL) || (mp->mnt_point == NULL)) {
        LOG_ERR("mount point not initialized!!\r\n");
        return -EINVAL;
    }

    if (listIS_CONTAINED_WITHIN(&fs_mnt_list, &(mp->node))) {
        LOG_ERR("file system already mounted!!\r\n");
        return -EBUSY;
    }

    len = strlen(mp->mnt_point);

    if ((len <= 1) || (mp->mnt_point[0] != '/')) {
        LOG_ERR("invalid mount point!!\r\n");
        return -EINVAL;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);

    ListItem_t *node = listGET_HEAD_ENTRY(&fs_mnt_list);
    ListItem_t const *end_node = listGET_END_MARKER(&fs_mnt_list);
    for (; node != end_node; node = listGET_NEXT(node)) {
        // printf("mount next one\r\n");
        itr = (struct fs_mount_t *)listGET_LIST_ITEM_OWNER(node);
        if (itr == NULL) {
            // printf("ERROR mnt is NULL\r\n");
            break;
        }
        if (len != itr->mountp_len) {
            // printf("len is different, %d:%d\r\n", len, itr->mountp_len);
            continue;
        }

        if (strncmp(mp->mnt_point, itr->mnt_point, len) == 0) {
            LOG_ERR("mount point already exists!!\r\n");
            rc = -EBUSY;
            goto mount_err;
        }
    }

    fs = fs_type_get(mp->type);
    // printf("%s, fs:%p\r\n",__func__, fs);
    if (fs == NULL) {
        LOG_ERR("requested file system type not registered!!\r\n");
        rc = -ENOENT;
        goto mount_err;
    }

    if (fs->mount == NULL) {
        LOG_ERR("fs type %d does not support mounting\r\n", mp->type);
        rc = -ENOTSUP;
        goto mount_err;
    }

    rc = fs->mount(mp);
    if (rc < 0) {
        LOG_ERR("fs mount error (%d)\r\n", rc);
        goto mount_err;
    }

    mp->mountp_len = len;
    mp->fs = fs;

    vListInitialiseItem(&mp->node);
    listSET_LIST_ITEM_OWNER(&mp->node, mp);
    vListInsertEnd(&fs_mnt_list, &(mp->node));
    LOG_DBG("fs mounted at %s\r\n", mp->mnt_point);

mount_err:
    xSemaphoreGive(mutex);
    return rc;
}

int vfs_unmount(struct fs_mount_t *mp)
{
    int rc = -EINVAL;

    if (mp == NULL)
        return rc;
    xSemaphoreTake(mutex, portMAX_DELAY);

    if (listLIST_ITEM_CONTAINER(&mp->node) != &fs_mnt_list) {
        LOG_ERR("fs not mounted (mp == %p)\r\n", mp);
        goto unmount_err;
    }

    if (mp->fs->unmount == NULL) {
        LOG_ERR("fs unmount not supported!!\r\n");
        rc = -ENOTSUP;
        goto unmount_err;
    }

    rc = mp->fs->unmount(mp);
    if (rc < 0) {
        LOG_ERR("fs unmount error (%d)\r\n", rc);
        goto unmount_err;
    }

    mp->fs = NULL;

    uxListRemove(&mp->node);

unmount_err:
    xSemaphoreGive(mutex);
    return rc;
}

int vfs_init(void)
{
    mutex = xSemaphoreCreateMutex();
    vListInitialise(&fs_mnt_list);
    return 0;
}

int init_fs(void)
{
    if (g_fs_mounted) {
        UART_SEND_DIRECT("FS already mounted.\r\n");
        return 0;
    }

    int err = 0;

    extern int vfs_init(void);
    extern int littlefs_init(void);

    extern void init_fdtable(void);
    init_fdtable();

    err = vfs_init();
    if (err == 0)
        // printf("vfs_init successfully.\r\n");
        UART_SEND_DIRECT("vfs_init successfully.\r\n");
    else {
        // printf("vfs_init failed. (%d).\r\n", err);
        UART_SEND_DIRECT("vfs_init failed. .\r\n");
        return err;
    }

    err = littlefs_init();
    if (err == 0)
        // printf("littlefs_init successfully.\r\n");
        UART_SEND_DIRECT("littlefs_init successfully.\r\n");
    else {
        // printf("littlefs_init failed. (%d).\r\n", err);
        UART_SEND_DIRECT("littlefs_init failed. .\r\n");
        return err;
    }

    err = vfs_mount(mp);
    if (err == 0) {
        g_fs_mounted = 1;
        UART_SEND_DIRECT("vfs_mount successfully.\r\n");
    } else {
        UART_SEND_DIRECT("vfs_mount failed. .\r\n");
        return err;
    }

    return err;
}

int is_fs_mounted(void)
{
    return g_fs_mounted;
}
