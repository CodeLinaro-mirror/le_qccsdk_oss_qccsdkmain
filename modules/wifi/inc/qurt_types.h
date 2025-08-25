#ifndef QURT_TYPES_H
#define QURT_TYPES_H

/*
 */

/**

@file qurt_types.h

@brief  definition of basic types, constants, preprocessor macros
*/

//#include "qurt_stddef.h"

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @addtogroup qurt_types
@{ */

#define QURT_TIME_NO_WAIT      0x00000000 /**< Return immediately without any waiting. */
#define QURT_TIME_WAIT_FOREVER 0xFFFFFFFF /**< Block until the operation is successful. */

/*=============================================================================
                        TYPEDEFS
=============================================================================*/

/** QuRT time types. */
typedef uint32_t qurt_time_t;

/** QuRT time unit types. */

/** @} */ /* end_addtogroup qurt_types */
#endif    /* QURT_TYPES_H */
