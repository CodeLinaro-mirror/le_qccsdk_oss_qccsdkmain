#include <stdlib.h>
#include <errno.h>

#include "nt_osal.h"

void *malloc(size_t size)
{
   return nt_osal_allocate_memory(size);     
}

void free(void *ptr)
{
    nt_osal_free_memory(ptr);
}

void *calloc(size_t count, size_t size)
{
    return nt_osal_calloc(count, size);
}
