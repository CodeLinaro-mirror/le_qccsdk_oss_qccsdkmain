/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_pdc.h"
#include "nt_pdc_driver.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include <string.h>

#ifdef NT_FN_PDC_

pdc_struct pdc_;

/**
 * <!-- nt_pdc_create_resource -->
 *
 * @brief Creates a resource based on the given definition and makes it
 * available to clients. After the new resource is linked into pdc_resources,
 * a request with value "initial_state" is issued to the resource.This may be used to perform
 * resource specific initializations.
 * @param definition: Pointer to a unpa_resource_definition.
 * @param initial_state: Initial_state of the resource. The driver function
 * is invoked with this value as "state" before the resource is made available to clients.
 * @return Returns a pointer to the created unpa_resource data structure.
 */
pdc_resource *nt_pdc_create_resource(pdc_resource_definition *definition, pdc_resource_state initial_state)
{
    pdc_resource *resource;
    CORE_VERIFY_PTR(definition);
    CORE_VERIFY(strlen(definition->name) < PDC_MAX_NAME_LEN);

    qurt_mutex_lock(&pdc_.lock);

    /* Verify that no resource with the given name is already defined/stubbed */
    resource = pdc_get_resource(definition->name);
    CORE_VERIFY(resource == NULL);

    resource = (pdc_resource *)nt_osal_calloc(1, sizeof(pdc_resource));
    CORE_VERIFY_PTR(resource);
    memset(resource, 0, sizeof(pdc_resource));

    resource->definition = definition;
    resource->vote_struct_head = NULL;
    resource->active_vote_max = definition->max_state;

    /* Initialize the resource's own lock */
    qurt_mutex_create(&resource->lock);

    resource->active_state = initial_state;
    // definition->driver_fcn( initial_state );

    /* Link in resource */
    resource->next = pdc_.resources;
    pdc_.resources = resource;

    qurt_mutex_unlock(&pdc_.lock);

    return resource;
}

/**
 * <!-- nt_pdc_update_resource -->
 * @brief Process the request from client
 */
void nt_pdc_update_resource(pdc_client *client, pdc_resource_state vote)
{
    pdc_resource *resource = client->resource;
    pdc_resource_state active_agg_vote;

    if (PDC_TRUE == nt_pdc_vote_processing(resource, client, vote)) {
        active_agg_vote = resource->definition->update_fcn(resource, client);

        if (active_agg_vote != resource->active_state) {
            resource->active_state = resource->definition->driver_fcn(active_agg_vote, client);
        }
    }

    qurt_mutex_unlock(&resource->lock);
}

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
pdc_client *pdc_create_client(char *client_name, char *resource_name, uint8_t client_type)
{
    pdc_resource *resource;
    pdc_client *client;
    pdc_resource_vote *start;
    pdc_resource_vote *resource_client_votes;

    CORE_VERIFY_PTR(client_name);
    CORE_VERIFY_PTR(resource_name);
    CORE_VERIFY(strlen(client_name) < PDC_MAX_NAME_LEN);

    resource = pdc_get_resource(resource_name);
    if (!resource) {
        return NULL;
    }

    client = (pdc_client *)nt_osal_calloc(1, sizeof(pdc_client));
    CORE_VERIFY_PTR(client);

    client->name = client_name;
    client->resource = resource;
    client->type = client_type;

    if (resource->vote_struct_head == NULL) {
        resource_client_votes = (pdc_resource_vote *)nt_osal_calloc(1, sizeof(pdc_resource_vote));
        CORE_VERIFY_PTR(resource_client_votes);

        resource_client_votes->client_type = client_type;
        resource_client_votes->client_name = client_name;
        resource_client_votes->vote = 0;

        resource->vote_struct_head = resource_client_votes;
        resource_client_votes->next = NULL;
    } else {
        start = resource->vote_struct_head;
        while (start->next != NULL) {
            start = start->next;
        }
        resource_client_votes = (pdc_resource_vote *)nt_osal_calloc(1, sizeof(pdc_resource_vote));
        CORE_VERIFY_PTR(resource_client_votes);

        resource_client_votes->client_name = client_name;
        resource_client_votes->client_type = client_type;
        resource_client_votes->vote = 0;
        resource_client_votes->next = NULL;
        start->next = resource_client_votes;
    }

    qurt_mutex_lock(&resource->lock);

    client->next = resource->clients;
    resource->clients = client;

    qurt_mutex_unlock(&resource->lock);

    return client;
}

/**
 * <!-- pdc_get_resource -->
 * @brief: Return the resource handle to the caller
 * @param resource_name: name of the resource which has to be checked
 * @return:  returns a pointer holding the address of the searched resource if successfull. NULL will
 * be returned otherwise
 */
pdc_resource *pdc_get_resource(char *resource_name)
{
    pdc_resource *resource;

    qurt_mutex_lock(&pdc_.lock);

    resource = pdc_.resources;

    while (resource) {
        if (0 == strncmp(resource_name, resource->definition->name, PDC_MAX_NAME_LEN + 1)) {
            break;
        }
        resource = resource->next;
    }

    qurt_mutex_unlock(&pdc_.lock);

    return resource;
}

/**
 * <!-- pdc_issue_request -->
 *
 * @brief To register the vote of a client to the resource on which it is attached to.
 * @param1 client: pointer to the client handle
 * @param2 vote: The required vote which has to be registered
 * the type of client you are issuing with.
 */
void pdc_issue_request(pdc_client *client, pdc_resource_state vote)
{
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(client->resource);
    if (vote > client->resource->active_vote_max) {
        return;
    }
    qurt_mutex_lock(&client->resource->lock);

    nt_pdc_update_resource(client, vote);
}

/**
 * <!-- nt_pdc_vote_processing -->
 *
 * @brief To register the vote issued to the resource from client into the data structure
 * param1 resource:  Pointer to the resource
 * param2 client: Pointer to the client
 * param3 vote: issued vote from the client
 * return PDC_TRUE if vote is successfully registered PDC_FALSE if the vote isn't registered.
 */
uint8_t nt_pdc_vote_processing(pdc_resource *resource, pdc_client *client, pdc_resource_state vote)
{
    CORE_VERIFY_PTR(resource);
    CORE_VERIFY_PTR(client);
    pdc_resource_vote *temp_head = resource->vote_struct_head;
    while (temp_head != NULL) {
        if (0 == strncmp(temp_head->client_name, client->name, PDC_MAX_NAME_LEN + 1)) {
            if (temp_head->vote != vote) {
                temp_head->vote = vote;
                return PDC_TRUE;
            }
        }
        temp_head = temp_head->next;
    }
    return PDC_FALSE;
}

/**
 * <!-- pdc_init -->
 *
 * @brief Initialize the mutex lock top serialize the resource accesses
 * and initialize the data structure to keep the resource handles
 * return void.
 */
void pdc_init(void)
{
    qurt_mutex_create(&pdc_.lock);
    pdc_.resources = NULL;
}

/**
 * <!-- pdc_init -->
 *
 * @brief Initialize the mutex lock top serialize the resource accesses
 * and initialize the data structure to keep the resource handles
 * return void.
 */
void nt_pdc_print_resource_votes(pdc_resource *resource)
{
    CORE_VERIFY_PTR(resource);
    pdc_resource_vote *temp_head = resource->vote_struct_head;
    while (temp_head != NULL) {
    }
}

#endif  // NT_FN_PDC_
