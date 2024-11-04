/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/****************************************************************
 * @file sort.h
 * @brief  A common Library declaration to handle sort Algorithms
 *
 *
 ******************************************************************/
#ifndef _SORT_H_
#define _SORT_H_

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define SORT_CONCAT_2(a, b) a##b
#define SORT_CONCAT_3(a, b, c) a##b##c

typedef enum sort_direction
{
    SORT_DIRECTION_ASSENDING  = 0,
    SORT_DIRECTION_DESSENDING = 1
} sort_direction_t;

typedef struct sort
{
    char *p_buffer;
    char *p_dummy_buff;
    size_t buffer_node_size;
    size_t size_buffer;
    size_t sort_param_offset;
    size_t sort_param_size;
} sort_t;

/*******************************************************************************
 * @brief Create a Sorting instance for user defined Structure/Unions
 *
 * @param _NAME     Name of the instance
 * @param _type     Structure data type
 * @param _buff     buffer to sort
 * @param _sort_param_offset_var    Name of parameter of structure to sort upon
 *
 *******************************************************************************/
#define SORT_INSTANCE_STRUCT(_NAME, _type, _buff, _sort_param_offset_var) \
    static _type SORT_CONCAT_3(sort_, _NAME, _dummy_buff)[sizeof(_type)]; \
    static sort_t SORT_CONCAT_3(sort_, _NAME, _inst) = {                  \
        .buffer_node_size = sizeof(_type),                                \
        .p_buffer = (char *)_buff,                                        \
        .size_buffer = sizeof(_buff),                                     \
        .sort_param_offset = offsetof(_type, _sort_param_offset_var),     \
        .sort_param_size = sizeof(((_type *)0)->_sort_param_offset_var),  \
        .p_dummy_buff = (char *)SORT_CONCAT_3(sort_, _NAME, _dummy_buff)};

 /******************************************************************************
  * @brief Create a Sorting instance for pre-defined data-types
  *
  * @param _NAME     Name of the instance
  * @param _type     data type
  * @param _buff     buffer to sort
  *
  *****************************************************************************/
#define SORT_INSTANCE_VAR(_NAME, _type, _buff)                           \
    static _type SORT_CONCAT_3(sort, _NAME, _dummy_buff)[sizeof(_type)]; \
    static sort_t SORT_CONCAT_3(sort_, _NAME, _inst) = {                 \
        .buffer_node_size = sizeof(_type),                               \
        .p_buffer = (char *)_buff,                                       \
        .size_buffer = sizeof(_buff),                                    \
        .sort_param_offset = 0,                                          \
        .sort_param_size = 0,                                            \
        .p_dummy_buff = (char *)SORT_CONCAT_3(sort_, _NAME, _dummy_buff)};

#define GET_SORT_INSTANCE(_Name) SORT_CONCAT_3(sort_, _Name, _inst)

/******************************************************************************
 * @brief Perform a bubble sort on the sort instance
 *
 * @param p_sort
 *****************************************************************************/
void sort_bubble(sort_t *p_sort, sort_direction_t dir);

#endif /*_SORT_H_*/
