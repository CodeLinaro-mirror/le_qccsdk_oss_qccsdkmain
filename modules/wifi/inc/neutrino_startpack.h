

#ifndef __NEUTRINOSTARTPACK_H__
#define __NEUTRINOSTARTPACK_H__

/* Define __ATTRIB_PACK in a compiler-specific way */
#if defined(__GNUC__)
        #define __ATTRIB_PACK           __attribute__ ((packed))
#endif

/* Choose whether compiler expects attribute before or after structure declaration */
#define PREPACK
#define POSTPACK                __ATTRIB_PACK

/* Handle structures that only need packing on 64-bit systems */
#if __LONG_MAX__ == __INT_MAX__
        /* 32-bit compilation */
        #define PREPACK64
        #define POSTPACK64
#else
        /* 64-bit compilation */
        #define PREPACK64 PREPACK
        #define POSTPACK64 POSTPACK
#endif

#endif /*__NEUTRINOSTARTPACK_H__ */
