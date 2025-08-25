/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief File descriptor table
 *
 * This file provides generic file descriptor table implementation, suitable
 * for any I/O object implementing POSIX I/O semantics (i.e. read/write +
 * aux operations).
 */

#include <errno.h>
#include <fcntl.h>
#include "fdtable.h"
#include <sys/types.h>
#include <stdarg.h>
#include "uart.h"
#define MAX_BUF_LEN 200

#define ARRAY_SIZE(array)                 (sizeof(array) / sizeof((array)[0]))
#define array_index_sanitize(index, size) ((index) < (size) ? (index) : (size)-1)

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

struct fd_entry {
    void *obj;
    const struct fd_op_vtable *vtable;
    SemaphoreHandle_t lock;
    volatile char used;
};

// static const struct fd_op_vtable stdinout_fd_op_vtable;

static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS] = {
#if 0
    {
        /*STDIN*/
        .vtable = &stdinout_fd_op_vtable,
        .used = 1,
    },
    {
        /*STDOUT*/
        .vtable = &stdinout_fd_op_vtable,
        .used = 1,
    },
    {
        /*STDERR*/
        .vtable = &stdinout_fd_op_vtable,
        .used = 1,
    },
#endif
    {0},
};

static SemaphoreHandle_t fdtable_lock;

void init_fdtable(void)
{
    UART_SEND_DIRECT("init_fdtable done.\r\n");
    fdtable_lock = xSemaphoreCreateMutex();
#if 0
    fdtable[0].lock = xSemaphoreCreateMutex();
    fdtable[1].lock = xSemaphoreCreateMutex();
    fdtable[2].lock = xSemaphoreCreateMutex();
#endif
    configASSERT(fdtable_lock != NULL);
}

static int z_fd_unref(int fd)
{
    fdtable[fd].obj = NULL;
    fdtable[fd].vtable = NULL;
    fdtable[fd].used = 0;
    vSemaphoreDelete(fdtable[fd].lock);
    fdtable[fd].lock = NULL;

    return 0;
}

static int _find_fd_entry(void)
{
    int fd;

    for (fd = 0; fd < (int)ARRAY_SIZE(fdtable); fd++) {
        if (fdtable[fd].used == 0) {
            fdtable[fd].used = 1;
            return fd;
        }
    }

    errno = ENFILE;
    return -1;
}

static int _check_fd(int fd)
{
    if (fd < 0 || fd >= (int)ARRAY_SIZE(fdtable)) {
        errno = EBADF;
        return -1;
    }

    fd = array_index_sanitize(fd, (int)ARRAY_SIZE(fdtable));

    if (fdtable[fd].used == 0) {
        errno = EBADF;
        return -1;
    }

    return 0;
}

void *z_get_fd_obj(int fd, const struct fd_op_vtable *vtable, int err)
{
    struct fd_entry *entry;

    if (_check_fd(fd) < 0) {
        return NULL;
    }

    entry = &fdtable[fd];

    if (vtable != NULL && entry->vtable != vtable) {
        errno = err;
        return NULL;
    }

    return entry->obj;
}

void *z_get_fd_obj_and_vtable(int fd, const struct fd_op_vtable **vtable, SemaphoreHandle_t **lock)
{
    struct fd_entry *entry;

    if (_check_fd(fd) < 0) {
        return NULL;
    }

    entry = &fdtable[fd];
    *vtable = entry->vtable;

    if (lock) {
        *lock = &entry->lock;
    }

    return entry->obj;
}

int z_reserve_fd(void)
{
    int fd;

    xSemaphoreTake(fdtable_lock, portMAX_DELAY);

    fd = _find_fd_entry();
    if (fd >= 0) {
        /* Mark entry as used, z_finalize_fd() will fill it in. */
        fdtable[fd].obj = NULL;
        fdtable[fd].vtable = NULL;
        fdtable[fd].lock = xSemaphoreCreateMutex();
    }

    xSemaphoreGive(fdtable_lock);

    return fd;
}

void z_finalize_fd(int fd, void *obj, const struct fd_op_vtable *vtable)
{
    /* Assumes fd was already bounds-checked. */
    fdtable[fd].obj = obj;
    fdtable[fd].vtable = vtable;
}

void z_free_fd(int fd)
{
    /* Assumes fd was already bounds-checked. */
    (void)z_fd_unref(fd);
}

int z_alloc_fd(void *obj, const struct fd_op_vtable *vtable)
{
    int fd;

    fd = z_reserve_fd();
    if (fd >= 0) {
        z_finalize_fd(fd, obj, vtable);
    }

    return fd;
}

#ifdef CONFIG_POSIX_FS_API

ssize_t _read(int fd, void *buf, size_t sz)
{
    // printf("in %s\n",__func__);
    ssize_t res;

    if (_check_fd(fd) < 0) {
        return -1;
    }

    xSemaphoreTake(fdtable[fd].lock, portMAX_DELAY);

    res = fdtable[fd].vtable->read(fdtable[fd].obj, buf, sz);

    xSemaphoreGive(fdtable[fd].lock);

    return res;
}

ssize_t _write(int fd, const void *buf, size_t sz)
{
    // printf("in %s\n",__func__);
    ssize_t res;

    if (_check_fd(fd) < 0) {
        return -1;
    }

    if (fd > 2)
        xSemaphoreTake(fdtable[fd].lock, portMAX_DELAY);

    res = fdtable[fd].vtable->write(fdtable[fd].obj, buf, sz);

    if (fd > 2)
        xSemaphoreGive(fdtable[fd].lock);

    return res;
}

int _close(int fd)
{
    int res;
    // printf("in %s\n",__func__);

    if (_check_fd(fd) < 0) {
        return -1;
    }

    xSemaphoreTake(fdtable[fd].lock, portMAX_DELAY);

    res = fdtable[fd].vtable->close(fdtable[fd].obj);

    xSemaphoreGive(fdtable[fd].lock);

    z_free_fd(fd);

    return res;
}

off_t _lseek(int fd, off_t offset, int whence)
{
    // printf("in %s\n",__func__);
    if (_check_fd(fd) < 0) {
        return -1;
    }

    return z_fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, ZFD_IOCTL_LSEEK, offset, whence);
}

#if 0
static ssize_t stdinout_read_vmeth(void *obj, void *buffer, size_t count)
{
        (void)obj;
        (void)buffer;
        (void)count;
	return 0;
}

static ssize_t stdinout_write_vmeth(void *obj, const void *buffer, size_t count)
{
    (void)obj;
    const char* buf = (const char*)buffer;
    UART_Send_direct((char *)buf, (uint32_t )count);
    return (ssize_t)count;
}

static int stdinout_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
        (void)obj;
        (void)request;
        (void)args;
	errno = EINVAL;
	return -1;
}

static const struct fd_op_vtable stdinout_fd_op_vtable = {
	.read = stdinout_read_vmeth,
	.write = stdinout_write_vmeth,
	.ioctl = stdinout_ioctl_vmeth,
};
#endif
#endif /* CONFIG_POSIX_FS_API */
