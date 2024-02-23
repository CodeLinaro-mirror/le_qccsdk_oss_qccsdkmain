/*
*/
#ifndef OS_AL_H_
#define OS_AL_H_

#include "data_path.h"

void *nt_dp_alloc(uint32_t size);
void nt_dp_free(void *ptr);

void *os_alloc(uint32_t size);
void os_free(void *ptr);

/** Flag needed for memory leak debugging */
#ifdef NT_FN_MEM_DEBUG		/* NT_FN_MEM_DEBUG */
void *nt_mMallocLineFile(size_t size, int line, char *file);
void nt_mFree(void *blockToFree);
void nt_mDisplayTable(void);
void nt_mClearTable(void);
#endif 						/* NT_FN_MEM_DEBUG */
/************************/
/* Remove after porting */



/************************/

#endif /* OS_AL_H_ */
