
#ifndef _QCCX_H_
#define _QCCX_H_

#if CONFIG_SOC_QCC730V1
    //version E3_07 same as hwio
    #include "qcc730v1_e307.h"
    #include "qcc730v1_posmask_e307.h"
#elif CONFIG_SOC_QCC730V2
    //version TAPEOUT_02
    #include "qcc730v2.h"
    #include "qcc730v2_posmask.h"
#else
    #error "SOC must be defined. See qccx.h."
#endif

#endif //_QCCX_H_
