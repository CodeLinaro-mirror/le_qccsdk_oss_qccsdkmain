/*
 */

#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"

#include "qapi_console.h"
#include "qcli_api.h"

QAPI_Console_Group_Handle_t QAPI_Console_Register_Command_Group(QAPI_Console_Group_Handle_t Parent_Group, const QAPI_Console_Command_Group_t *Command_Group)
{
    return QCLI_Register_Command_Group(Parent_Group, Command_Group);
}

QAPI_Console_Handle_t QAPI_Console_Register_Command(QAPI_Console_Group_Handle_t __attribute__((__unused__)) Parent_Group, const QAPI_Console_Command_t __attribute__((__unused__)) *Command)
{
	return (QAPI_Console_Handle_t)0;
}

