
 /*
 */
#ifndef CORE_SYSTEM_INC_FERM_PROF_DEMO_H_
#define CORE_SYSTEM_INC_FERM_PROF_DEMO_H_
#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_console.h"
qapi_Status_t prof_demo_init();
qapi_Status_t prof_demo_config(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t prof_demo_start();
qapi_Status_t prof_demo_stop();
qapi_Status_t prof_demo_deinit();
#endif
