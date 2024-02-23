
#ifndef _NVM_COMMON_H_
#define _NVM_COMMON_H_

 /*-------------------------------------------------------------------------
  * Include header files from standard path
  *-----------------------------------------------------------------------*/
#include <stdint.h>


 /*-------------------------------------------------------------------------
  * Preprocessor Definitions, Constants, and Type Declarations
  *-----------------------------------------------------------------------*/

//Register Read and Write
#define HW_REG_WR( _reg, _val ) ( *( ( volatile uint32_t * ) (_reg ) ) ) =  _val ;\
		__asm volatile("dsb" ::: "memory");

#define HW_REG_RD( _reg ) ( *( ( volatile uint32_t* )( _reg ) ) )

#define FRN_SOFTWARE_RESET(){\
		HW_REG_WR( QWLAN_PMU_SYS_SOFT_RESET_REQ_REG, QWLAN_PMU_SYS_SOFT_RESET_REQ_SYS_SOFT_RESET_REQ_MASK);}

#endif
