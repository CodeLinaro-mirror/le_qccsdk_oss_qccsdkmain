/*
 * prng.h
 *
 */
/*
*/

#ifndef CORE_SYSTEM_HCAL_INC_NT_PRNG_H_
#define CORE_SYSTEM_HCAL_INC_NT_PRNG_H_

#include "nt_osal.h"
#include "nt_common.h"

int8_t nt_prng_init(void);
uint32_t nt_pget_rng( void );
NT_BOOL nt_wlan_hw_prng_get(uint8_t *ptr, uint16_t len );



#endif /* CORE_SYSTEM_HCAL_INC_NT_PRNG_H_ */
