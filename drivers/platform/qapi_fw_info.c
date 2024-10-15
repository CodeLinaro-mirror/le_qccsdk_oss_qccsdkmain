/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/*****************************************************************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_version.h"


/**
 * This API allows user to retrieve build information from system.
 * @param[out]    info          Value retrieved from system.
 * @return        QAPI_OK -- Requested parameter retrieved from the system.
 *                Non-Zero value -- Parameter retrieval failed.
 */
qapi_Status_t qapi_Get_FW_Info(qapi_FW_Info_t *info)
{
    if( info == NULL)
        return QAPI_ERR_INVALID_PARAM;


    info->qapi_Version_Number = __QAPI_ENCODE_VERSION(QAPI_VERSION_MAJOR, QAPI_VERSION_MINOR, QAPI_VERSION_NIT);
    info->crm_Build_Number = CRM_BUILD_NUM;
    return QAPI_OK;
}

