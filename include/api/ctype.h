#ifndef _CTYPE_H_
#define _CTYPE_H_

#include "_ansi.h"
#include <sys/cdefs.h>

#if __POSIX_VISIBLE >= 200809 || __MISC_VISIBLE || defined (_COMPILING_NEWLIB)
#include <sys/_locale.h>
#endif

_BEGIN_STD_C

int isalnum (int __c);
int isalpha (int __c);
int iscntrl (int __c);
int isdigit (int __c);
int isgraph (int __c);
int islower (int __c);
int isprint (int __c);
int ispunct (int __c);
int isspace (int __c);
int isupper (int __c);
int isxdigit (int __c);
int tolower (int __c);
int toupper (int __c);

#include <ctype_internal.h>

_END_STD_C

#endif /* _CTYPE_H_ */
