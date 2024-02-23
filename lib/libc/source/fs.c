
#ifdef NT_FN_LFS

#include "posix/unistd.h"
#include "posix/fcntl.h"
#include "lfs.h"

typedef struct
{
    int fd;
    lfs_file_t file;
} fd_info_t;

#define MAX_FILES 20
fd_info_t fd_table[MAX_FILES];

extern lfs_t lfs_init;

int open(const char *name, int flags, ...)
{
    int i;
    for(i = 0; i< MAX_FILES; i++)
    {
        if(fd_table[i].fd == 0)
        {
            break;
        }
    }

    if(i == MAX_FILES)
    {
        return -1;
    }

    lfs_file_t file;
    int lfs_flags = 0;

    if (flags & O_RDONLY) 
    {
        lfs_flags |= LFS_O_RDONLY;
    }

    if (flags & O_WRONLY )
    {
        lfs_flags |= LFS_O_WRONLY;
    }

    if (flags & O_RDWR)
    {
        lfs_flags |= LFS_O_RDWR;
    }

    if (flags & O_CREAT)
    {
        lfs_flags |= LFS_O_CREAT;
    }

    if (flags & O_EXCL)
    {
        lfs_flags |= LFS_O_EXCL;
    }

    if (flags & O_TRUNC)
    {
        lfs_flags |= LFS_O_TRUNC;
    }

    if (flags & O_APPEND)
    {
        lfs_flags |= LFS_O_APPEND;
    }

    int err = lfs_file_open(&lfs_init, &file, name, lfs_flags);

    if (err)
    {
        //handle err
        return -1;
    } else {
        int fd = i+1;
        fd_table[i].file = file;
        return fd;
    }
}

int close(int fd)
{
    if( fd < MAX_FILES )
    {
        lfs_file_t file = fd_table[fd].file;
        return lfs_file_close(&lfs_init, (&file));
    } else {
        return -1;
    }
}

ssize_t write(int fd, const void *buffer, size_t count)
{
   lfs_ssize_t res = lfs_file_write(&lfs_init, &(fd_table[fd].file), buffer, count);
   return (ssize_t) res;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    lfs_ssize_t res = lfs_file_read(&lfs_init, &(fd_table[fd].file), buffer, count);
    return (ssize_t) res;
}

off_t lseek(int fd, off_t offset, int whence)
{
   lfs_soff_t res = lfs_file_seek(&lfs_init, &(fd_table[fd].file), offset, whence);
   if (res < 0)
       return res;
   else 
       return (off_t)res;
}
#endif

