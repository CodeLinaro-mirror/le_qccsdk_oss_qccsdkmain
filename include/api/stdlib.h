/*
 * stdlib.h
 *
 * Definitions for common types, variables, and functions.
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <stdlib_internal.h>

_BEGIN_STD_C

void	*calloc(size_t, size_t);
void	free (void *);
void	*malloc(size_t);

_END_STD_C


#endif /* _STDLIB_H_ */
