/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file qapi_version.h
 *
 * @brief QAPI version information
 *
 * @details This file provides defintions for the QAPI/SDK version.
 */

#ifndef __QAPI_VERSION_H__ // [
#define __QAPI_VERSION_H__

#include "qapi_status.h"

#define QAPI_CHIP_VERSION 0x0101
#define QAPI_BUILD_VERSION 0x07D0

#define QAPI_CHIP_VERSION_STR "0101"
#define QAPI_BUILD_VERSION_STR "07D0"

/** @addtogroup qapi_build_info
@{ */

#define QAPI_VERSION_MAJOR                                     (3)
#define QAPI_VERSION_MINOR                                     (0)
#define QAPI_VERSION_NIT                                       (0)

#define __QAPI_VERSION_MAJOR_MASK                             (0xff000000)
#define __QAPI_VERSION_MINOR_MASK                             (0x00ff0000)
#define __QAPI_VERSION_NIT_MASK                               (0x0000ffff)

#define __QAPI_VERSION_MAJOR_SHIFT                             (24)
#define __QAPI_VERSION_MINOR_SHIFT                             (16)
#define __QAPI_VERSION_NIT_SHIFT                               (0)

#define __QAPI_ENCODE_VERSION(__major__, __minor__, __nit__)   (((__major__) << __QAPI_VERSION_MAJOR_SHIFT) | \
                                                                ((__minor__) << __QAPI_VERSION_MINOR_SHIFT) | \
                                                                ((__nit__)   << __QAPI_VERSION_NIT_SHIFT))

/*----------------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------------*/
/**
 * Data structure used by application to get build information.
 */
typedef struct {
    uint32_t qapi_Version_Number;
    /**< qapi version number */
    uint32_t crm_Build_Number;
    /**< CRM build number */
	char crm_full_version[16];
    /**< CRM build full version */
} qapi_FW_Info_t;

/*----------------------------------------------------------------------------------
 * Function Declarations and Documentation
 * -------------------------------------------------------------------------------*/
/**
 * This API allows user to retrieve version information from system. \n
 *
 * @param[out]    info          Value retrieved from system.
 *
 * @return        QAPI_OK -- Requested parameter retrieved from the system. \n
 *                Non-Zero value -- Parameter retrieval failed.
 *
 * @dependencies          None.
 */
qapi_Status_t qapi_Get_FW_Info(qapi_FW_Info_t *info);

//const uint32_t qapi_Version_Number = __QAPI_ENCODE_VERSION(QAPI_VERSION_MAJOR, QAPI_VERSION_MINOR, QAPI_VERSION_NIT);

#endif // ] #ifndef __QAPI_VERSION_H__
