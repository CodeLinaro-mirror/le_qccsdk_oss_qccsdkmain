/*
	assert.h
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <assert_internal.h>

#undef assert

#ifdef NDEBUG           /* required by ANSI standard */
# define assert(__e) ((void)0)
#else
# define assert(__e) ((__e) ? (void)0 : __assert_func (__FILE__, __LINE__, \
						       __ASSERT_FUNC, #__e))
#endif /* !NDEBUG */
#ifdef __cplusplus
}
#endif
