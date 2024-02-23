/*========================================================================

* @file ani_test.h
* @brief ANI Functionality Unit Test header
* ======================================================================*/
#ifndef ANI_TEST_H
#define ANI_TEST_H

/*-----------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "nt_flags.h"

#ifdef SUPPORT_UNIT_TEST_CMD
#ifdef FERMION_ANI_SW_SUPPORT

/*------------------------------------------------------------------------
* Function Declarations and Documentation
* ----------------------------------------------------------------------*/
void ani_unit_test_handler(uint16_t num_args, uint32_t* args);

#endif /* #ifdef SUPPORT_UNIT_TEST_CMD */
#endif /* #ifdef FERMION_ANI_SW_SUPPORT */
#endif /* ANI_TEST_H */
