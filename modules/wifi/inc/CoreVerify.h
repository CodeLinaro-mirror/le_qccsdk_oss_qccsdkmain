#ifndef COREVERIFY_H
#define COREVERIFY_H



#define CORE_VERIFY(x)                                                    \
    do                                                                    \
    {                                                                     \
        if (0 == (x)){;}						                          \
    } while(0)
#define CORE_VERIFY_PTR(ptr)      CORE_VERIFY(NULL != (ptr))
//#define CORE_DAL_VERIFY(dal_fcn)  CORE_VERIFY(DAL_SUCCESS == (dal_fcn))

#endif // COREVERIFY_H

