/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*============================================================================
@file unpa_internal.h

uNPA Internal data structures, types etc.


============================================================================*/
#ifndef _UNPA_INTERNAL_H
#define _UNPA_INTERNAL_H

#include "malloc.h"
//#include "FreeRTOSConfig.h"
#include <string.h>
#include "unpa_resource.h"
#include "CoreVerify.h"
//#include "CoreUtil.h"
//#include "platform_wrapper.h"
#include "qurt_mutex.h"
#include <stdlib.h>
/*----------------------------------------------------------------------------
 * Defines/Enums
 * -------------------------------------------------------------------------*/

#define UNPA_MAX_STUBS 5
#define UNPA_LOG(w)
#define warning_ignore \
    (a) if ((a) == 0)  \
    {                  \
        for (;;) {     \
            ;          \
        }              \
    }
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
//#define platform_mutex_init ( x )
//	qurt_mutex_create ( x )
//#define platform_mutex_lock ( y )
//	qurt_mutex_lock ( y)
//#define platform_mutex_unlock ( z )
//	qurt_mutex_unlock ( z)
/*----------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------*/

typedef uint32_t UNPA_LOG_T;

typedef struct unpa_struct {
    /* The list of defined UNPA resources */
    unpa_resource *resources;

    /* The list of stubbed UNPA resources */
    const char *stubs[UNPA_MAX_STUBS];

    /* Mutex to serialise access to the above lists */
    qurt_mutex_t lock;

    /* UNPA activity is logged into this log */
    UNPA_LOG_T log;
} unpa_struct;

/*----------------------------------------------------------------------------
 * Extern references
 * -------------------------------------------------------------------------*/

extern unpa_struct unpa;

/*----------------------------------------------------------------------------
 * Function declarations
 * -------------------------------------------------------------------------*/

/**
 * <!-- unpa_resource_has_attribute -->
 * @brief Returns TRUE if the resource has the given attribute.
 */
uint32_t unpa_resource_has_attribute(unpa_resource *resource, uint32_t attribute);

/**
 * <!-- unpa_get_resource -->
 * Returns the unpa_resource* with the given name or NULL.
 */
unpa_resource *unpa_get_resource(const char *resource_name);

/**
 * <!-- unpa_update_resource -->
 * @brief Process the request from client
 */
void unpa_update_resource(unpa_client *client);

/**
 * @brief Returns a mask with client types supported by the given
 * (UNPA-provided) update function
 */
uint32_t unpa_get_supported_client_types(unpa_resource_update_fcn update_fcn);

/**
 * <!-- unpa_min_update_fcn -->
 * @brief Returns the minimum of all active non-zero requests or 0, if all
 * active_requests are 0.
 */
unpa_resource_state unpa_min_update_fcn(unpa_resource *resource, unpa_client *client);

/**
 * <!-- unpa_max_update_fcn -->
 * @brief Returns the maximum of all active requests.
 */
unpa_resource_state unpa_max_update_fcn(unpa_resource *resource, unpa_client *client);

/**
 * <!-- unpa_binary_update_fcn -->
 * @brief Returns 1 if there is atleast one active non-zero request, or 0.
 * IMPORTANT: This is different from how the plugin/update_fcn is defined
 * in full NPA, where we return active_max if there is atleast one
 * non-zero vote. Modifying this for uNPA saves 2 words per resource.
 */
unpa_resource_state unpa_binary_update_fcn(unpa_resource *resource, unpa_client *client);

#endif /* _UNPA_INTERNAL_H */
