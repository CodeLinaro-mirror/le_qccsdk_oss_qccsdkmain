/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_PDC_H_
#define CORE_SYSTEM_INC_NT_PDC_H_

#include "qurt_internal.h"
#ifdef NT_FN_PDC_
/*---------------------------------------------MACROS-----------------------------------------------------*/
#define CORE_VERIFY_PTR(ptr) CORE_VERIFY(NULL != (ptr))

#define CORE_VERIFY(x)  \
    do {                \
        if (0 == (x)) { \
            ;           \
        }               \
    } while (0)

#define PDC_MAX_NAME_LEN  16 /* Names are restricted to this many chars, incl. '\0' */
#define PDC_TRUE          0x1
#define PDC_FALSE         0x0
#define PDC_ACTIVE_CLIENT 0x0  // Client type for active state
#define PDC_SLEEP_CLIENT  0x1  // Client type for sleep state
/*--------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------TYPEDEFS----------------------------------------------------*/
typedef uint32_t pdc_resource_state;

/*--------------------------------------------------------------------------------------------------------*/

/*-----------------------------------------Structure Definitions------------------------------------------*/
// Structure to serialize the mutex locks

/*--------------------------------------------Globals-----------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------*/

typedef struct pdc_client {
    /* Name is limited to UNPA_MAX_NAME_LEN chars, including the '\0' */
    const char *name;

    /* Client type */
    uint8_t type : 8;

    /* Request attributes */
    uint32_t request_attr : 24;

    /* Active request from client */
    uint32_t active_request;

    /* Resource to which this client is a client to; will be set by UNPA
       during register */
    struct pdc_resource *resource;

    /* Client-specific resource data; allows resource authors to associate
       client-specific data to this structure */
    void *resource_data;

    /* Pointer to the next client to "resource" */
    struct pdc_client *next;
} pdc_client;

/* An update function defines how the resource aggregates requests */
typedef pdc_resource_state (*pdc_resource_update_fcn)(struct pdc_resource *resource, pdc_client *client);

/* A driver function defines how the resource applies requests */
typedef pdc_resource_state (*pdc_resource_driver_fcn)(pdc_resource_state state, pdc_client *client);

/* A UNPA resource definition may be placed in code or RO memory */
typedef struct pdc_resource_definition {
    /* Length of "name", incl. the '\0', must be < UNPA_MAX_NAME_LEN */
    char *name;

    /* Pointer to the function used to aggregate resource requests */
    pdc_resource_update_fcn update_fcn;

    /* Pointer to the function that applies the aggregated request */
    pdc_resource_driver_fcn driver_fcn;

    /* Max possible resource state; used to initialize or reset active_max
       in unpa_resource */
    pdc_resource_state max_state;

    /* Bitmask of supported client types */
    //  uint32_t client_types : 8; //@ TODO check if it is needed or not

    /* Resource attributes */
    //  uint32_t attributes : 24; //@ TODO check if it is needed or not
} pdc_resource_definition;

/* This data structure represents the dynamic state of the PDC resource */
typedef struct pdc_resource {
    /* Pointer to a pdc_resource_definition data structure */
    pdc_resource_definition *definition;

    /* Linked list of clients registered with this resource */
    pdc_client *clients;

    /* The active state of the resource; for sysPM based resources, this
       represents the AS setting */
    pdc_resource_state active_state;

    /* The sleep state of the resource; for sysPM based resources, this
       represents the SS setting */
    // pdc_resource_state sleep_state;

    /* The aggregation of requests of a particular type */
    // pdc_resource_state agg_state[3];

    /* The aggregation of resource requests will be clipped to active_max,
       before being passed into the driver function */
    pdc_resource_state active_vote_max;

    /* In multi-threaded operating modes, serializes requests to the resource */
    qurt_mutex_t lock;

    /* Allows resource authors to associate any user data with this resource */
    //  void *user_data;

    /* Pointer to the next resource in the global unpa_resources */
    struct pdc_resource *next;

    /* Pointer to client vote structure in the resource */
    struct pdc_resource_vote *vote_struct_head;
} pdc_resource;

typedef struct pdc_struct {
    /* The list of defined PDC resources */
    pdc_resource *resources;

    /* The list of stubbed UNPA resources */
    //  const char *stubs[UNPA_MAX_STUBS];

    /* Mutex to serialise access to the above lists */
    qurt_mutex_t lock;

    /* UNPA activity is logged into this log */
    // UNPA_LOG_T log;
} pdc_struct;

typedef struct pdc_resource_vote {
    uint8_t client_type;
    char *client_name;
    uint32_t vote;  // TODO Alt : Current_vote
    struct pdc_resource_vote *next;
} pdc_resource_vote;
/*---------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------Function
 * declarations---------------------------------------------------*/

/**
 * <!-- nt_pdc_create_resource -->
 *
 * @brief Creates a resource based on the given definition and makes it
 * available to clients. After the new resource is linked into pdc_resources,
 * a request with value "initial_state" is issued to the resource with a
 * special client_type NPA_CLIENT_INITIALIZE. This may be used to perform
 * resource specific initializations.
 * The active_max of the resource is set to definition->max
 *
 * @param definition: Pointer to a pdc_resource_definition.
 * @param initial_state: Initial_state of the resource. The driver function
 * is invoked with this value as "state" and client->type set to
 * UNPA_CLIENT_INITIALIZE before the resource is made available to clients.
 *
 * @return Returns a pointer to the created unpa_resource data structure.
 */
pdc_resource *nt_pdc_create_resource(pdc_resource_definition *definition, pdc_resource_state initial_state);
/**
 * <!-- nt_pdc_vote_processing -->
 *
 * @brief To register the vote issued to the resource from client into the data structure
 * param1 resource:  Pointer to the resource
 * param2 client: Pointer to the client
 * param3 vote: issued vote from the client
 * return PDC_TRUE if vote is successfully registered PDC_FALSE if the vote isn't registered.
 */
uint8_t nt_pdc_vote_processing(pdc_resource *resource, pdc_client *client, pdc_resource_state vote);
/**
 * <!-- pdc_get_resource -->
 * @brief: Return the resource handle to the caller
 * @param resource_name: name of the resource which has to be checked
 * @return:  returns a pointer holding the address of the searched resource if successfull. NULL will
 * be returned otherwise
 */
pdc_resource *pdc_get_resource(char *resource_name);
/**
 * <!-- pdc_issue_request -->
 *
 * @brief To register the vote of a client to the resource on which it is attached to.
 * @param1 client: pointer to the client handle
 * @param2 vote: The required vote which has to be registered
 * the type of client you are issuing with.
 */
void pdc_issue_request(pdc_client *client, pdc_resource_state vote);
/**
 * <!-- pdc_create_client -->
 *
 * @brief Create a client to a resource; you cannot issue requests to resources
 * without first creating a client to it. If a resource with the given name
 * is defined, a new pdc_client structure
 * will be created and and a pointer to it returned to caller.
 * If the client cannot be created, NULL is returned.
 *
 * @param client_name: Name of the client; length, including the '\0',
 * must be < PDC_MAX_NAME_LEN.
 * @param resource_name: Name of the resource to create a client to
 * @param client type: Type of client needs to be created. PDC_SLEEP_CLIENT for MCU Sleep
 * and PDC_ACTIVE_CLIENT for active state
 * @return If successful, a pointer to a pdc_client structure; else, NULL
 */
pdc_client *pdc_create_client(char *client_name, char *resource_name, uint8_t client_type);
/**
 * <!-- pdc_init -->
 *
 * @brief Initialize the mutex lock top serialize the resource accesses
 * and initialize the data structure to keep the resource handles
 * return void.
 */
void pdc_init(void);
void nt_pdc_print_resource_votes(pdc_resource *resource);
/*---------------------------------------------------------------------------------------------------------------------*/

#endif  // NT_FN_PDC_
#endif  /* CORE_SYSTEM_INC_NT_PDC_H_ */
