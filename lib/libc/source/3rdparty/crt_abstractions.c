/* 
 * For strncpy_s(): https://github.com/Azure/azure-c-shared-utility/blob/master/src/crt_abstractions.c
 * Copyright (c) Microsoft. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the ""Software""), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */

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
