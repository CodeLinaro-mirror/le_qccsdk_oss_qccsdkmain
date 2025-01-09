// For strncpy_s(): https://github.com/Azure/azure-c-shared-utility/blob/master/src/crt_abstractions.c
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "safeAPI.h"
#include "FreeRTOS_POSIX/errno.h"

#if !defined (_TRUNCATE)
#define _TRUNCATE ((size_t)-1)
#endif  /* !defined (_TRUNCATE) */

#if !defined STRUNCATE
#define STRUNCATE       80
#endif  /* !defined (STRUNCATE) */

/*Codes_SRS_CRT_ABSTRACTIONS_99_025: [strncpy_s shall copy the first N characters of src to dst, where N is the lesser of MaxCount and the length of src.]*/
int strncpy_s(char* dst, size_t dstSizeInBytes, const char* src, size_t maxCount)
{
    int result;
    int truncationFlag = 0;
    /*Codes_SRS_CRT_ABSTRACTIONS_99_020: [If dst is NULL, the error code returned shall be EINVAL and dst shall not be modified.]*/
    if (dst == NULL)
    {
        result = EINVAL;
    }
    /*Codes_SRS_CRT_ABSTRACTIONS_99_021: [If src is NULL, the error code returned shall be EINVAL and dst[0] shall be set to 0.]*/
    else if (src == NULL)
    {
        dst[0] = '\0';
        result = EINVAL;
    }
    /*Codes_SRS_CRT_ABSTRACTIONS_99_022: [If the dstSizeInBytes is 0, the error code returned shall be EINVAL and dst shall not be modified.]*/
    else if (dstSizeInBytes == 0)
    {
        result = EINVAL;
    }
    else
    {
        size_t srcLength = strlen(src);
        if (maxCount != _TRUNCATE)
        {
            /*Codes_SRS_CRT_ABSTRACTIONS_99_041: [If those N characters will fit within dst (whose size is given as dstSizeInBytes) and still leave room for a null terminator, then those characters shall be copied and a terminating null is appended; otherwise, strDest[0] is set to the null character and ERANGE error code returned.]*/
            if (srcLength > maxCount)
            {
                srcLength = maxCount;
            }

            /*Codes_SRS_CRT_ABSTRACTIONS_99_023: [If dst is not NULL & dstSizeInBytes is smaller than the required size for the src string, the error code returned shall be ERANGE and dst[0] shall be set to 0.]*/
            if (srcLength + 1 > dstSizeInBytes)
            {
                dst[0] = '\0';
                result = ERANGE;
            }
            else
            {
                (void)strlcpy(dst, src, srcLength);
                dst[srcLength] = '\0';
                /*Codes_SRS_CRT_ABSTRACTIONS_99_018: [strncpy_s shall return Zero upon success]*/
                result = 0;
            }
        }
        /*Codes_SRS_CRT_ABSTRACTIONS_99_026: [If MaxCount is _TRUNCATE (defined as -1), then as much of src as will fit into dst shall be copied while still leaving room for the terminating null to be appended.]*/
        else
        {
            if (srcLength + 1 > dstSizeInBytes )
            {
                srcLength = dstSizeInBytes - 1;
                truncationFlag = 1;
            }
            (void)strlcpy(dst, src, srcLength);
            dst[srcLength] = '\0';
            result = 0;
        }
    }

    /*Codes_SRS_CRT_ABSTRACTIONS_99_019: [If truncation occurred as a result of the copy, the error code returned shall be STRUNCATE.]*/
    if (truncationFlag == 1)
    {
        result = STRUNCATE;
    }

    return result;
}
