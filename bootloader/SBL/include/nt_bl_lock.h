#ifndef _BOOT_LOCK_H_
#define	 _BOOT_LOCK_H_

#include "Fermion_seq_hwioreg.h"

void pbl_exit_configuration(void);
void pbl_force_unlock_jtag(void);

typedef void (*pbl_exit_configuration_t)(void);
typedef void (*pbl_force_unlock_jtag_t)(void);

typedef struct {
	pbl_exit_configuration_t	pbl_exit_configuration_pfn;
	pbl_force_unlock_jtag_t		pbl_force_unlock_jtag_pfn;
} nt_bl_lock_ind_t;

#endif
